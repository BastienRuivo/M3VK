#include "handler/Handlers.h"
#include "application/ApplicationInfo.h"
#include <cstdint>
#include <cstring>
#include <set>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "application/DebugLayer.h"

VkCommandPoolHandler::VkCommandPoolHandler(uint32_t queueFamilyIndex)
: Handler<VkCommandPool>(), _queueFamilyIndex(queueFamilyIndex)
{
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
}

VkDeviceHandler::VkDeviceHandler(VkInstance instance, VkSurfaceKHR windowSurface, const std::vector<const char*>& deviceExtensions)
{
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

    VkPhysicalDeviceSynchronization2Features sync2Features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext = nullptr,
        .synchronization2 = VK_TRUE
    };

    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3Features{};
    extendedDynamicState3Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
    extendedDynamicState3Features.pNext = &sync2Features;
    extendedDynamicState3Features.extendedDynamicState3RasterizationSamples = VK_TRUE;
    extendedDynamicState3Features.extendedDynamicState3ColorBlendEnable = VK_TRUE;
    extendedDynamicState3Features.extendedDynamicState3ColorBlendEquation = VK_TRUE;
    extendedDynamicState3Features.extendedDynamicState3ColorWriteMask = VK_TRUE;
    extendedDynamicState3Features.extendedDynamicState3PolygonMode = VK_TRUE;
    extendedDynamicState3Features.extendedDynamicState3AlphaToCoverageEnable = VK_TRUE;
    extendedDynamicState3Features.extendedDynamicState3DepthClampEnable = VK_TRUE;
    extendedDynamicState3Features.extendedDynamicState3LogicOpEnable = VK_TRUE;
    extendedDynamicState3Features.extendedDynamicState3AlphaToOneEnable = VK_TRUE;

    VkPhysicalDeviceExtendedDynamicState2FeaturesEXT extendedDynamicState2Features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT,
        .pNext = &extendedDynamicState3Features,
        .extendedDynamicState2 = VK_TRUE
    };

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
        .pNext = &extendedDynamicState2Features,
        .extendedDynamicState = VK_TRUE
    };

    VkPhysicalDeviceShaderObjectFeaturesEXT shaderObjectFeatures
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT,
        .pNext = &extendedDynamicStateFeatures,
        .shaderObject = VK_TRUE,
    };

    VkPhysicalDeviceVulkan11Features vulkan11Features
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .pNext = &shaderObjectFeatures,
        .shaderDrawParameters = VK_TRUE
    };

    // enable bindless support
    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
        .pNext = &vulkan11Features
    };

    descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE; // different instance in the same wave can access different textures
    descriptorIndexingFeatures.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE; // descriptor can be updated while bound
    descriptorIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeatures.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
    descriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE; // slot can be empty
    descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE; // actual count set at all location
    descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE; // unsized array in shaders

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
        .fillModeNonSolid = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
    };

    VkDeviceCreateInfo deviceCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &dynamicRenderingFeatures,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),

        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,

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
    VkFunctions::Instance(); // force loading of vk ext functions
}

VkDeviceHandler::~VkDeviceHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroyDevice(_internal, nullptr);
}

VkFenceHandler::VkFenceHandler()
{
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
}

VkInstanceHandler::VkInstanceHandler()
{
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
    std::vector<const char*> missingExtensions;
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
            missingExtensions.push_back(requiredExtensions[i]);
        }
    }

    if(missingExtensions.size() > 0)
    {
        for(const char* missingExtension : missingExtensions)
        {
            DebugLayer::Log(DebugLayer::LogType::ERROR, "Missing VK Extension : " + std::string(missingExtension));
        }
        throw std::runtime_error("Missing required VK Extensions");
    }

    if(!DebugLayer::CheckValidationLayerSupport())
    {
        throw std::runtime_error("Validation layer requested but not available !");
    }

    // 1. Declare the synchronization feature you want to force-enable
    VkValidationFeatureEnableEXT enabledFeatures[] = {
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
    };

    // 2. Wrap it inside the validation features structure
    VkValidationFeaturesEXT validationFeatures{};
    validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    validationFeatures.enabledValidationFeatureCount = 1;
    validationFeatures.pEnabledValidationFeatures = enabledFeatures;

    VkInstanceCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &validationFeatures,
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

    ApplicationInfo::Get()._vkInstance = _internal;
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
}

VkQueueHandler::VkQueueHandler(VkQueueHandler::QueueTypeEnum queueType)
{
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

VkSamplerHandler::VkSamplerHandler(VkFilter oversampling, VkFilter undersampling)
{
    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    // Mag -> Oversampling, Min -> Undersampling
    createInfo.magFilter = oversampling;
    createInfo.minFilter = undersampling;

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
}

VkSurfaceHandler::VkSurfaceHandler(VkInstance instance, GLFWwindow* pWindow)
{
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
}

VkPipelineLayoutHandler::VkPipelineLayoutHandler( std::span<const VkDescriptorSetLayout> descriptorLayouts, std::span<const VkPushConstantRange> pushConstantRanges)
{
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
}
