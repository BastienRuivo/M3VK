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

UserInterface::UserInterface(GLFWwindow* pWindow, const SwapChain& swapChain, vk::Queue graphicsQueue, vk::CommandPool cmdPool)
{
    const uint32_t size = 200;
    vk::DescriptorPoolSize poolSizes[] = {
        { vk::DescriptorType::eSampler, size },
        { vk::DescriptorType::eCombinedImageSampler, size },
        { vk::DescriptorType::eSampledImage, size },
        { vk::DescriptorType::eStorageImage, size },
        { vk::DescriptorType::eUniformTexelBuffer, size },
        { vk::DescriptorType::eStorageTexelBuffer, size },
        { vk::DescriptorType::eUniformBuffer, size },
        { vk::DescriptorType::eStorageBuffer, size },
        { vk::DescriptorType::eUniformBufferDynamic, size },
        { vk::DescriptorType::eStorageBufferDynamic, size },
        { vk::DescriptorType::eInputAttachment, size }
    };

    vk::DescriptorPoolCreateInfo poolInfo = vk::DescriptorPoolCreateInfo{}
        .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
        .setMaxSets(size * IM_ARRAYSIZE(poolSizes))
        .setPoolSizes(poolSizes);

    if (ApplicationInfo::Device().createDescriptorPool(&poolInfo, nullptr, &_imGuiPool) != vk::Result::eSuccess) {
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
    initInfo.ImageCount = static_cast<uint32_t>(swapChain.Images.size());
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.CheckVkResultFn = [](VkResult err) {
        if (err == 0) return;
        fprintf(stderr, "[ImGui Vulkan Error] VkResult = %d\n", err);
    };

    VkFormat imageFormat = static_cast<VkFormat>(swapChain.GetImageFormat());

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
    ApplicationInfo::Device().waitIdle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    ApplicationInfo::Device().destroyDescriptorPool(_imGuiPool);
}

UserInterfaceImageSet::UserInterfaceImageSet(const GraphicsImage& image, vk::ImageLayout layout, vk::Sampler sampler)
{
    RessourceUsage usage = image.Usage();
    _usage = usage;

    uint32_t count = RessourceUsageCount(usage);

    _internals.resize(count);

    for (uint32_t i = 0; i < RessourceUsageCount(usage); i++)
    {
        const auto& texture = image[i];
        _internals[i] = ImGui_ImplVulkan_AddTexture(sampler, texture.View(), static_cast<VkImageLayout>(layout));
    }
}

UserInterfaceImageSet::UserInterfaceImageSet(const BindingManager& manager, BindlessTexture handle, vk::ImageLayout layout, vk::Sampler sampler)
{
    RessourceUsage usage = handle.Usage;
    _usage = usage;

    uint32_t count = RessourceUsageCount(usage);
    _internals.resize(count);


    for (uint32_t i = 0; i < count; i++)
    {
        const auto& bindlessTexture = handle.Texture(manager);
        _internals[i] = ImGui_ImplVulkan_AddTexture(sampler, bindlessTexture.View(), static_cast<VkImageLayout>(layout));
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
