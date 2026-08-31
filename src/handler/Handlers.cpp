#include "handler/Handlers.h"
#include "application/VkExtManager.h"
#include "application/ApplicationInfo.h"
#include <cstdint>
#include <cstring>
#include <set>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "application/DebugLayer.h"

VkCommandPoolHandler::VkCommandPoolHandler(uint32_t queueFamilyIndex)
: Handler<vk::CommandPool>(), _queueFamilyIndex(queueFamilyIndex)
{
    vk::CommandPoolCreateInfo poolInfo = vk::CommandPoolCreateInfo{}
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
        .setQueueFamilyIndex(_queueFamilyIndex);

    vk::Result result = ApplicationInfo::Device().createCommandPool(&poolInfo, nullptr, &_internal);
    if(result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create command pool !");
    }
}

VkCommandPoolHandler::~VkCommandPoolHandler()
{
    if(!_internal) return;

    ApplicationInfo::Device().destroyCommandPool(_internal);
}

VkDeviceHandler::VkDeviceHandler(vk::SurfaceKHR windowSurface, const std::vector<const char*>& deviceExtensions)
{
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueIds =
    {
        ApplicationInfo::Get().GetGraphicsQueueId(),
        ApplicationInfo::Get().GetPresentQueueId(),
        ApplicationInfo::Get().GetTransferQueueId()
    };

    float queuePriority = 1.0f;
    for(uint32_t queueId : uniqueQueueIds)
    {
        queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{}
            .setQueueFamilyIndex(queueId)
            .setQueueCount(1)
            .setPQueuePriorities(&queuePriority));
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

    vk::PhysicalDeviceFeatures deviceFeatures = vk::PhysicalDeviceFeatures{}
        .setSampleRateShading(VK_TRUE)
        .setMultiDrawIndirect(VK_TRUE)
        .setDrawIndirectFirstInstance(VK_TRUE)
        .setFillModeNonSolid(VK_TRUE)
        .setSamplerAnisotropy(VK_TRUE);

    // enabledLayerCount/ppEnabledLayerNames are deprecated (device-level layers no longer exist) and
    // default to 0/nullptr, matching what this used to set explicitly
    vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo{}
        .setPNext(&dynamicRenderingFeatures)
        .setQueueCreateInfos(queueCreateInfos)
        .setPEnabledExtensionNames(deviceExtensions)
        .setPEnabledFeatures(&deviceFeatures);

    vk::Result deviceCreation = vk::PhysicalDevice(ApplicationInfo::PhysicalDevice()).createDevice(
        &deviceCreateInfo, nullptr, &_internal);
    if(deviceCreation != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create VK Logical Device !");
    }

    VkExtManager::InitDevice(_internal);
    ApplicationInfo::Get()._device = _internal;
}

VkDeviceHandler::~VkDeviceHandler()
{
    if(!_internal) return;

    _internal.destroy();
}

VkFenceHandler::VkFenceHandler()
{
    // Create the queue in the "Signaled" state to ensure the first frame won't wait eternally for a fence that is not signaled, thus preventing an infinit loop
    vk::FenceCreateInfo fenceCreateInfo = vk::FenceCreateInfo{}
        .setFlags(vk::FenceCreateFlagBits::eSignaled);

    vk::Result result = ApplicationInfo::Device().createFence(&fenceCreateInfo, nullptr, &_internal);
    if(result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Can't create fence");
    }
}

VkFenceHandler::~VkFenceHandler()
{
    if(!_internal) return;

    ApplicationInfo::Device().destroyFence(_internal);
}

void VkFenceHandler::Wait(uint64_t timeout) const
{
    (void)ApplicationInfo::Device().waitForFences(1, &_internal, VK_TRUE, timeout);
}

void VkFenceHandler::Reset() const
{
    (void)ApplicationInfo::Device().resetFences(1, &_internal);
}

VkSemaphoreHandler::VkSemaphoreHandler()
{
    vk::SemaphoreCreateInfo semaphoreCreateInfo{};

    vk::Result result = ApplicationInfo::Device().createSemaphore(&semaphoreCreateInfo, nullptr, &_internal);
    if(result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Can't create image available semaphore");
    }
}

VkSemaphoreHandler::~VkSemaphoreHandler()
{
    if(!_internal) return;

    ApplicationInfo::Device().destroySemaphore(_internal);
}

vk::raii::Instance M3VKConstruct::MakeInstance(const vk::raii::Context& context,
        const std::string_view name,
        const uint32_t appVersion,
        const uint32_t engineVersion,
        const uint32_t apiVersion)
{
    VkExtManager::InitLoader();

    vk::ApplicationInfo appInfo = vk::ApplicationInfo{}
        .setPApplicationName(name.data())
        .setApplicationVersion(appVersion)
        .setEngineVersion(engineVersion)
        .setApiVersion(apiVersion);

    std::vector<vk::ExtensionProperties> supportedExtensions = vk::enumerateInstanceExtensionProperties();

    if(DebugLayer::Enabled)
    {
#ifdef M3VK_VERBOSE_LOG
        DebugLayer::Log(DebugLayer::LogType::INFO, "List of actives VK Extensions");
#endif
        for(const vk::ExtensionProperties& extension : supportedExtensions)
        {
#ifdef M3VK_VERBOSE_LOG
            DebugLayer::Log(DebugLayer::LogType::INFO, std::string("\t - ") + extension.extensionName.data());
#endif
        }
    }

    std::vector<const char *> requiredExtensions = M3VKConstruct::Helper::GetRequiredExtensions();
    std::vector<const char *> missingExtensions;
    for(int i = 0; i < requiredExtensions.size(); ++i)
    {
        bool isPresent = false;
        for(const vk::ExtensionProperties& extension : supportedExtensions)
        {
            if(strcmp(requiredExtensions[i], extension.extensionName.data()) == 0)
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

    vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo{}
        .setPApplicationInfo(&appInfo)
        .setEnabledExtensionCount(static_cast<uint32_t>(requiredExtensions.size()))
        .setPpEnabledExtensionNames(requiredExtensions.data());

    // ensure it is not destroyed before vkCreateInstance
    vk::DebugUtilsMessengerCreateInfoEXT debugInfoCreate;
    DebugLayer::SetupCreateInfo(createInfo, debugInfoCreate);

    vk::raii::Instance instance(context, createInfo);

    VkExtManager::InitInstance(instance);
    ApplicationInfo::Get()._vkInstance = instance;

    return instance;
}

std::vector<const char *> M3VKConstruct::Helper::GetRequiredExtensions()
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

    _internal = vk::Device(ApplicationInfo::Device()).getQueue(family, 0);
    _queueFamilyIndex = family;
    _type = queueType;
}

VkQueueHandler::~VkQueueHandler()
{
}

VkQueueHandler::VkQueueHandler(VkQueueHandler && other) noexcept
{
    _internal = std::exchange(other._internal, nullptr);
    _queueFamilyIndex = std::exchange(other._queueFamilyIndex, 0);
    _type = std::exchange(other._type, QueueTypeEnum::Graphics);
}

VkQueueHandler& VkQueueHandler::operator=(VkQueueHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = std::exchange(other._internal, nullptr);
        _queueFamilyIndex = std::exchange(other._queueFamilyIndex, 0);
        _type = std::exchange(other._type, QueueTypeEnum::Graphics);
    }
    return *this;
}

VkSamplerHandler::VkSamplerHandler(vk::Filter oversampling, vk::Filter undersampling, vk::SamplerMipmapMode mipmapMode, bool hasAniso)
{
    // Mag -> Oversampling, Min -> Undersampling
    // what to do when reading OOB (repeat, clamp, mirror...)
    // Anisotropy -> Avoiding blur caused by mipmapping by doing clever more sampling
    // Comparaison operation for shadow mapping apparently
    vk::SamplerCreateInfo createInfo = vk::SamplerCreateInfo{}
        .setMagFilter(static_cast<vk::Filter>(oversampling))
        .setMinFilter(static_cast<vk::Filter>(undersampling))
        .setAddressModeU(vk::SamplerAddressMode::eRepeat)
        .setAddressModeV(vk::SamplerAddressMode::eRepeat)
        .setAddressModeW(vk::SamplerAddressMode::eRepeat)
        .setAnisotropyEnable(hasAniso ? VK_TRUE : VK_FALSE)
        .setMaxAnisotropy(hasAniso ? ApplicationInfo::Get().GetProperties().limits.maxSamplerAnisotropy : 0.0f)
        .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
        .setUnnormalizedCoordinates(VK_FALSE)
        .setCompareEnable(VK_FALSE)
        .setCompareOp(vk::CompareOp::eAlways)
        .setMipmapMode(static_cast<vk::SamplerMipmapMode>(mipmapMode))
        .setMipLodBias(0.0f)
        .setMinLod(0.0f)
        .setMaxLod(VK_LOD_CLAMP_NONE);

    vk::Result result = ApplicationInfo::Device().createSampler(&createInfo, nullptr, &_internal);
    if(result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Can't create sampler");
    }
}
VkSamplerHandler::~VkSamplerHandler()
{
    if(!_internal) return;

    ApplicationInfo::Device().destroySampler(_internal);
}

VkSurfaceHandler::VkSurfaceHandler(GLFWwindow* pWindow)
{
    VkSurfaceKHR rawSurface;
    if(glfwCreateWindowSurface(ApplicationInfo::Instance(), pWindow, nullptr, &rawSurface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create windows surface !");
    }
    _internal = rawSurface;
}

VkSurfaceHandler::~VkSurfaceHandler()
{
    if(!_internal) return;
    ApplicationInfo::Instance().destroySurfaceKHR(_internal);
}

VkPipelineLayoutHandler::VkPipelineLayoutHandler( std::span<const vk::DescriptorSetLayout> descriptorLayouts, std::span<const vk::PushConstantRange> pushConstantRanges)
{
    vk::PipelineLayoutCreateInfo layoutCreateInfo = vk::PipelineLayoutCreateInfo{}
        .setSetLayouts(descriptorLayouts)
        .setPushConstantRanges(pushConstantRanges);

    if(ApplicationInfo::Device().createPipelineLayout(&layoutCreateInfo, nullptr, &_internal) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create VK Layout !");
    }
}
VkPipelineLayoutHandler::~VkPipelineLayoutHandler()
{
    if(!_internal) return;
    ApplicationInfo::Device().destroyPipelineLayout(_internal);
}
