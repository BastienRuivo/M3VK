#include "application/UserInterface.h"
#include "allocation/RessourceUsage.h"
#include "application/ApplicationInfo.h"
#include "imgui_impl_vulkan.h"
#include <cstdint>

void UserInterface::StartFrame() const
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
};

UserInterface::UserInterface(GLFWwindow* pWindow, const SwapChain& swapChain, VkQueue graphicsQueue, VkCommandPool cmdPool)
{
    const uint32_t size = 200;
    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, size },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, size },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, size },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, size },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, size },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, size },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, size },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, size },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, size },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, size },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, size }
    };

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = size * IM_ARRAYSIZE(poolSizes);
    poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
    poolInfo.pPoolSizes = poolSizes;

    if (vkCreateDescriptorPool(ApplicationInfo::Device(), &poolInfo, nullptr, &_imGuiPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create ImGui descriptor pool.");
    }


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(pWindow, true);

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = ApplicationInfo::Instance();
    initInfo.PhysicalDevice = ApplicationInfo::PhysicalDevice();
    initInfo.Device = ApplicationInfo::Device();
    initInfo.QueueFamily = ApplicationInfo::GetGraphicsQueueId();
    initInfo.Queue = graphicsQueue;
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = _imGuiPool;
    initInfo.Subpass = 0;
    initInfo.MinImageCount = swapChain.MinImageCount();
    initInfo.ImageCount = swapChain.Images.Size();
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.CheckVkResultFn = [](VkResult err) {
        if (err == 0) return;
        fprintf(stderr, "[ImGui Vulkan Error] VkResult = %d\n", err);
    };

    VkFormat imageFormat = swapChain.GetImageFormat();

    VkPipelineRenderingCreateInfo pipelineRenderingInfo = {};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &imageFormat; // e.g., VK_FORMAT_B8G8R8A8_SRGB
    pipelineRenderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;     // Change if ImGui draws to a depth buffer
    pipelineRenderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    initInfo.UseDynamicRendering = true;
    initInfo.PipelineRenderingCreateInfo = pipelineRenderingInfo;

    ImGui_ImplVulkan_Init(&initInfo);
    ImGui_ImplVulkan_CreateFontsTexture();
}

UserInterface::~UserInterface()
{
    vkDeviceWaitIdle(ApplicationInfo::Device());

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool(ApplicationInfo::Device(), _imGuiPool, nullptr);
}

UserInterfaceImageSet::UserInterfaceImageSet(const GraphicsImage& image, VkImageLayout layout, VkSampler sampler)
{
    RessourceUsage usage = image.Usage();
    _usage = usage;

    uint32_t count = RessourceUsageCount(usage);

    _internals.resize(count);

    for (uint32_t i = 0; i < RessourceUsageCount(usage); i++)
    {
        const auto& texture = image[i];
        _internals[i] = ImGui_ImplVulkan_AddTexture(sampler, texture.View(), layout);
    }
}

UserInterfaceImageSet::UserInterfaceImageSet(const BindingManager& manager, BindlessTexture handle, VkImageLayout layout, VkSampler sampler)
{
    RessourceUsage usage = handle.Usage;
    _usage = usage;

    uint32_t count = RessourceUsageCount(usage);
    _internals.resize(count);


    for (uint32_t i = 0; i < count; i++)
    {
        const auto& bindlessTexture = handle.Texture(manager);
        _internals[i] = ImGui_ImplVulkan_AddTexture(sampler, bindlessTexture.View(), layout);
    }
}

UserInterfaceImageSet::~UserInterfaceImageSet()
{
    for (uint32_t i = 0; i < _internals.size(); i++)
    {
        ImGui_ImplVulkan_RemoveTexture(_internals[i]);
    }
}

UserInterfaceImageSet::UserInterfaceImageSet(UserInterfaceImageSet&& other) noexcept
{
    _internals = std::move(other._internals);
}

UserInterfaceImageSet& UserInterfaceImageSet::operator=(UserInterfaceImageSet&& other) noexcept
{
    _internals = std::move(other._internals);
    return *this;
}
