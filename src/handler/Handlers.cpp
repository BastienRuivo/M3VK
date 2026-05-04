#include "handler/Handlers.h"
#include "application/ApplicationInfo.h"
#include <cstdint>
#include <cstring>
#include <set>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include "application/DebugLayer.h"
#include "asset/Shader.h"
#include "asset/Vertex.h"

VkCommandPoolHandler::VkCommandPoolHandler(uint32_t queueFamilyIndex)
: Handler<VkCommandPool>(), _queueFamilyIndex(queueFamilyIndex)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkCommandPoolHandler Creation !");
#endif

    VkCommandPoolCreateInfo poolInfo
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = _queueFamilyIndex
    };

    if(vkCreateCommandPool(ApplicationInfo::Device(), &poolInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool !");
    }
}

VkCommandPoolHandler::~VkCommandPoolHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroyCommandPool(ApplicationInfo::Device(), _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkCommandPoolHandler Destroyed !");
#endif
}

VkDeviceHandler::VkDeviceHandler(VkSurfaceKHR windowSurface, const std::vector<const char*>& deviceExtensions)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkDeviceHandler Creation !");
#endif

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueIds =
    {
        ApplicationInfo::Get().GetGraphicsQueueId(),
        ApplicationInfo::Get().GetPresentQueueId(),
        ApplicationInfo::Get().GetTransferQueueId()
    };

    float queuePriority = 1.0f;
    for(uint32_t queueId : uniqueQueueIds)
    {
        VkDeviceQueueCreateInfo queueCreateInfo
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueId,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        };
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceVulkan11Features vulkan11Features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .shaderDrawParameters = VK_TRUE
    };

    // enable bindless support
    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
        .pNext = &vulkan11Features,

        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE, // different instance in the same wave can access different textures
        .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE, // descriptor can be updated while bound
        .descriptorBindingPartiallyBound = VK_TRUE, // slot can be empty
        .descriptorBindingVariableDescriptorCount = VK_TRUE, // actual count set at all location
        .runtimeDescriptorArray = VK_TRUE, // unsized array in shaders
    };

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .pNext = &descriptorIndexingFeatures,
        .dynamicRendering = VK_TRUE,
    };

    VkPhysicalDeviceFeatures deviceFeatures
    {
        .sampleRateShading = VK_TRUE,
        .multiDrawIndirect = VK_TRUE,
        .drawIndirectFirstInstance = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
    };

    VkDeviceCreateInfo deviceCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &dynamicRenderingFeatures,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),

        .enabledLayerCount = DebugLayer::Enabled ? static_cast<uint32_t>(DebugLayer::ValidationLayer.size()) : 0,
        .ppEnabledLayerNames = DebugLayer::Enabled ? DebugLayer::ValidationLayer.data() : nullptr,

        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &deviceFeatures
    };

    VkResult deviceCreation = vkCreateDevice(ApplicationInfo::PhysicalDevice(), &deviceCreateInfo, nullptr, &_internal);
    if(deviceCreation != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create VK Logical Device !");
    }

    ApplicationInfo::Get()._device = _internal;
}

VkDeviceHandler::~VkDeviceHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroyDevice(_internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkDeviceHandler Destroyed !");
#endif
}

VkFenceHandler::VkFenceHandler()
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkFenceHandler Creation !");
#endif


    VkFenceCreateInfo fenceCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT // Create the queue in the "Signaled" state to ensure the first frame won't wait eternally for a fence that is not signaled, thus preventing an infinit loop
    };

    if(vkCreateFence(ApplicationInfo::Device(), &fenceCreateInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Can't create fence");
    }

}

VkFenceHandler::~VkFenceHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroyFence(ApplicationInfo::Device(), _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkFenceHandler Destroyed !");
#endif
}

void VkFenceHandler::Wait(uint64_t timeout) const
{
    vkWaitForFences(ApplicationInfo::Device(), 1, &_internal, VK_TRUE, timeout);
}

void VkFenceHandler::Reset() const
{
    vkResetFences(ApplicationInfo::Device(), 1, &_internal);
}

VkSemaphoreHandler::VkSemaphoreHandler()
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkSemaphoreHandler Creation !");
#endif


    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if(vkCreateSemaphore(ApplicationInfo::Device(), &semaphoreCreateInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Can't create image available semaphore");
    }
}

VkSemaphoreHandler::~VkSemaphoreHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroySemaphore(ApplicationInfo::Device(), _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkSemaphoreHandler Destroyed !");
#endif
}

VkInstanceHandler::VkInstanceHandler()
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkInstanceHandler creation !");
#endif

    VkApplicationInfo appInfo
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "M3VK",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_4
    };

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, supportedExtensions.data());

    if(DebugLayer::Enabled)
    {
#ifdef M3VK_VERBOSE_LOG
        DebugLayer::Log(DebugLayer::LogType::INFO, "List of actives VK Extensions");
#endif
        for(const VkExtensionProperties& extension : supportedExtensions)
        {
#ifdef M3VK_VERBOSE_LOG
            DebugLayer::Log(DebugLayer::LogType::INFO, std::string("\t - ") + extension.extensionName);
#endif
        }
    }

    std::vector<const char *> requiredExtensions = GetRequiredExtensions();
    for(int i = 0; i < requiredExtensions.size(); ++i)
    {
        bool isPresent = false;
        for(const VkExtensionProperties& extension : supportedExtensions)
        {
            if(strcmp(requiredExtensions[i], extension.extensionName) == 0)
            {
                isPresent = true;
                break;
            }
        }

        if(!isPresent)
        {
            throw std::runtime_error("Vulkan Extension" + std::string(requiredExtensions[i]) + " Is not supported but required for GLFW");
        }
    }

    if(!DebugLayer::CheckValidationLayerSupport())
    {
        throw std::runtime_error("Validation layer requested but not available !");
    }

    VkInstanceCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data(),
    };

    // ensure it is not destroyed before vkCreateInstance
    VkDebugUtilsMessengerCreateInfoEXT debugInfoCreate;
    DebugLayer::SetupCreateInfo(createInfo, debugInfoCreate);

    VkResult result = vkCreateInstance(&createInfo, nullptr, &_internal);
    if(result != VK_SUCCESS)
    {
        if(result == VK_ERROR_LAYER_NOT_PRESENT)
        {
            throw std::runtime_error("Error : A VK Layer is not present on computer");
        }
        else
        {
            throw std::runtime_error("Failed to create VK_Instance");
        }
    }
}

std::vector<const char *> VkInstanceHandler::GetRequiredExtensions() const
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if(DebugLayer::Enabled)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}
VkInstanceHandler::~VkInstanceHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroyInstance(_internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkInstanceHandler Destroyed !");
#endif
}

VkPipelineHandler::VkPipelineHandler(const VkExtent2D& appExtent, VkSampleCountFlagBits msaaSampleCount, VkPipelineLayout pipelineLayout, VkFormat swapChainFormat, VkFormat depthFormat)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkPipelineHandler Creation !");
#endif
    Shader shader;

    VkPipelineShaderStageCreateInfo shadersStagesCreateInfo[2] = {
        {},
        {}
    };

    shadersStagesCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shadersStagesCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shadersStagesCreateInfo[0].module = shader.VertexShader;
    shadersStagesCreateInfo[0].pName = "main";

    shadersStagesCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shadersStagesCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shadersStagesCreateInfo[1].module = shader.FragmentShader;
    shadersStagesCreateInfo[1].pName = "main";

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo pipelineStateCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    VkVertexInputBindingDescription bindingDescription = Vertex::GetBindingDescription();
    auto attributeDescription = Vertex::GetAttributeDescription();


    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size()),
        .pVertexAttributeDescriptions = attributeDescription.data()
    };

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        // For later use (When i will choose if i still want to make a cube world like thingy) maybe i can try strip ?
        // like in opengl, this turn 4 index like 1234 into 2 triangle (123, 324) and this can save me some place (bandwith <3)
        // theres also some element buffer shit to setup here later
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = false
    };

    VkViewport viewport
    {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)appExtent.width,
        .height = (float)appExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    // Region to rasterize pixels on
    VkRect2D scissors
    {
        .offset = {0, 0},
        .extent = appExtent
    };

    VkPipelineViewportStateCreateInfo viewportCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizeCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        // if true, clamp near and far object to the planes instead of discarding them
        .depthClampEnable = false,
        // disable geometry from going to the rasterizer stage ??
        .rasterizerDiscardEnable = false,
        // VK_POLYGON_MODE_LINE for wireframe later maybe ? or this can be another feature to enable, check later
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f
    };

    VkPipelineMultisampleStateCreateInfo msaaCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = msaaSampleCount,
        .sampleShadingEnable = VK_TRUE,
        .minSampleShading = 0.2f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    // Blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment
    {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
        .colorBlendOp = VK_BLEND_OP_ADD, // Optional
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
        .alphaBlendOp = VK_BLEND_OP_ADD, // Optional
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY, // Optional
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f } // Optional
    };

    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,

        // Can discard pixels fragment that aren't in this interval
        .depthBoundsTestEnable = VK_FALSE,

        // Depth Stencil Not used for now
        .stencilTestEnable = VK_FALSE,
        .front{},
        .back{},

        .minDepthBounds = 0.0f, // Optional
        .maxDepthBounds = 1.0f, // Optional
    };

    VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapChainFormat,
        .depthAttachmentFormat = depthFormat,
    };

    VkGraphicsPipelineCreateInfo pipelineCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipelineRenderingCreateInfo,
        .stageCount = 2,

        // Graphics pipeline
        .pStages = shadersStagesCreateInfo,
        .pVertexInputState = &vertexInputCreateInfo,
        .pInputAssemblyState = &inputAssemblyCreateInfo,
        .pViewportState = &viewportCreateInfo,
        .pRasterizationState = &rasterizeCreateInfo,
        .pMultisampleState = &msaaCreateInfo,
        .pDepthStencilState = &depthStencilCreateInfo,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &pipelineStateCreateInfo,

        .layout = pipelineLayout,
        .subpass = 0,

        // can be use to switch between parent pipeline (less expensive theorically if simillar)
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };



    if(vkCreateGraphicsPipelines(ApplicationInfo::Device(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline !");
    }
}
VkPipelineHandler::~VkPipelineHandler()
{
    if(_internal == VK_NULL_HANDLE) return;
    vkDestroyPipeline(ApplicationInfo::Device(), _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkPipelineHandler Destroyed !");
#endif
}

VkPipelineLayoutHandler::VkPipelineLayoutHandler( std::span<const VkDescriptorSetLayout> descriptorLayouts, std::span<const VkPushConstantRange> pushConstantRanges)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkPipelineLayoutHandler Creation !");
#endif


    VkPipelineLayoutCreateInfo layoutCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(descriptorLayouts.size()),
        .pSetLayouts = descriptorLayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
        .pPushConstantRanges = pushConstantRanges.data()
    };

    if(vkCreatePipelineLayout(ApplicationInfo::Device(), &layoutCreateInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create VK Layout !");
    }
}
VkPipelineLayoutHandler::~VkPipelineLayoutHandler()
{
    if(_internal == VK_NULL_HANDLE) return;
    vkDestroyPipelineLayout(ApplicationInfo::Device(), _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkPipelineLayoutHandler Destroyed !");
#endif
}

VkQueueHandler::VkQueueHandler(VkQueueHandler::QueueTypeEnum queueType)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkQueueHandler Creation !");
#endif

    uint32_t family;

    switch (queueType)
    {
        case Present: family = ApplicationInfo::Get().GetPresentQueueId(); break;
        case Graphics: family = ApplicationInfo::Get().GetGraphicsQueueId(); break;
        case Transfer: family = ApplicationInfo::Get().GetTransferQueueId(); break;
        default: throw std::runtime_error("Unimplemented graphics queue type");
    }

    vkGetDeviceQueue(ApplicationInfo::Device(),family, 0, &_internal);
    _queueFamilyIndex = family;
    _type = queueType;
}

VkQueueHandler::~VkQueueHandler()
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkQueueHandler Destroyed !");
#endif
}

VkQueueHandler::VkQueueHandler(VkQueueHandler && other) noexcept
{
    _internal = std::exchange(other._internal, VK_NULL_HANDLE);
    _queueFamilyIndex = std::exchange(other._queueFamilyIndex, 0);
    _type = std::exchange(other._type, QueueTypeEnum::Graphics);
}

VkQueueHandler& VkQueueHandler::operator=(VkQueueHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = std::exchange(other._internal, VK_NULL_HANDLE);
        _queueFamilyIndex = std::exchange(other._queueFamilyIndex, 0);
        _type = std::exchange(other._type, QueueTypeEnum::Graphics);
    }
    return *this;
}

VkSamplerHandler::VkSamplerHandler()
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkSamplerHandler Creation !");
#endif

    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    // Mag -> Oversampling, Min -> Undersampling
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;

    // what to do when reading OOB (repeat, clamp, mirror...)
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    // Anisotropy -> Avoiding blur caused by mipmapping by doing clever more sampling
    createInfo.anisotropyEnable = VK_TRUE;
    createInfo.maxAnisotropy = ApplicationInfo::Get().GetProperties().limits.maxSamplerAnisotropy;

    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;

    // Comparaison operation for shadow mapping apparently
    createInfo.compareEnable = VK_FALSE;
    createInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = VK_LOD_CLAMP_NONE;

    if(vkCreateSampler(ApplicationInfo::Device(), &createInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Can't create sampler");
    }
}
VkSamplerHandler::~VkSamplerHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroySampler(ApplicationInfo::Device(), _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkSamplerHandler Destroyed !");
#endif
}

VkSurfaceHandler::VkSurfaceHandler(VkInstance instance, GLFWwindow* pWindow)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkSurfaceHandler creation !");
#endif

    _instance = instance;
    if(glfwCreateWindowSurface(_instance, pWindow, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create windows surface !");
    }
}

VkSurfaceHandler::~VkSurfaceHandler()
{
    if(_internal == VK_NULL_HANDLE) return;
    vkDestroySurfaceKHR(_instance, _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkSurfaceHandler Destroyed !");
#endif
}
