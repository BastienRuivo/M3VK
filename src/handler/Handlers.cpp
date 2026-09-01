#include "handler/Handlers.h"
#include "application/ApplicationHelper.h"
#include "application/VkExtManager.h"
#include "application/ApplicationInfo.h"
#include <cstdint>
#include <cstring>
#include <set>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "application/DebugLayer.h"

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
    // Note: ApplicationInfo::Instance() isn't bound here - `instance` is a local about to be moved
    // into Application::_instance, so its address isn't stable yet. DebugLayer, the next thing built
    // from it, does the actual binding once it receives the real, permanently-addressed object.

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

vk::raii::SurfaceKHR M3VKConstruct::MakeSurface(const vk::raii::Instance& instance, GLFWwindow *pWindow)
{
    VkSurfaceKHR rawSurface;
    if(glfwCreateWindowSurface(*instance, pWindow, nullptr, &rawSurface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create windows surface !");
    }

    return vk::raii::SurfaceKHR(instance, rawSurface);
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

bool M3VKConstruct::Helper::CheckPhysicalDeviceExtensionSupport(vk::PhysicalDevice device, const std::vector<const char *>& deviceExtensions)
{
    uint32_t extensionCount = 0;
    vk::Result result = device.enumerateDeviceExtensionProperties(nullptr, &extensionCount, nullptr);

    if(result != vk::Result::eSuccess)
    {
        DebugLayer::Log(DebugLayer::ERROR, "Fail to query device extension properties size");
    }

    std::vector<vk::ExtensionProperties> properties(extensionCount);
    result = device.enumerateDeviceExtensionProperties(nullptr, &extensionCount, properties.data());

    if(result != vk::Result::eSuccess)
    {
        DebugLayer::Log(DebugLayer::ERROR, "Fail to enumerate device extension properties");
    }

    for(const auto& extension : deviceExtensions)
    {
        bool foundExtension = false;
        for(const vk::ExtensionProperties& property : properties)
        {
            if(strcmp(extension, property.extensionName.data()) == 0)
            {
                foundExtension = true;
                break;
            }
        }
        if(!foundExtension)
        {
            if(DebugLayer::Enabled)
            {
                DebugLayer::Log(DebugLayer::LogType::ERROR, (std::string("Extension not supported : ") + std::string(extension)).c_str());
            }
            return false;
        }
    }

    return true;
}

uint32_t M3VKConstruct::Helper::FindMemoryType(vk::PhysicalDevice device, uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memoryProperties;
    device.getMemoryProperties(&memoryProperties);

    for (uint32_t memoryType = 0; memoryType < memoryProperties.memoryTypeCount; ++memoryType)
    {
        // is suitable for buffer & writable by CPU
        if((typeFilter & (1 << memoryType)) && ((memoryProperties.memoryTypes[memoryType].propertyFlags & properties) == properties))
        {
            return memoryType;
        }
    }

    throw std::runtime_error("Can't find suitable memory type for buffer");
}

int M3VKConstruct::Helper::ScorePhysicalDeviceSuitability(vk::PhysicalDevice device, vk::SurfaceKHR windowSurface, const std::vector<const char *>& deviceExtensions, vk::PhysicalDeviceProperties& deviceProperties, QueueFamilyIds& familyIds)
{
    device.getProperties(&deviceProperties);

    // support of addtionnal feature (texture compression, 64bit double, multi viewport rendering)
    vk::PhysicalDeviceFeatures deviceFeatures = device.getFeatures();

    int score = 0;


    familyIds = QueueFamilyIds::QueryQueueFamilies(device, windowSurface);

    bool areAllRequiredExtensionsSupported = M3VKConstruct::Helper::CheckPhysicalDeviceExtensionSupport(device, deviceExtensions);

    ApplicationHelper::SwapChainSupportDetails swapChainDetails = ApplicationHelper::QuerySwapChainSupportDetail(device, windowSurface);

    // Mandatory feature, if any return 0 and this will be the only way to have 0 score meaning there's no suitable GPU
    if(!QueueFamilyIds::AreAllQueueAvailable(familyIds)
        || !areAllRequiredExtensionsSupported
        || !swapChainDetails.CheckSwapChainSupportAdequate()
        || !deviceFeatures.samplerAnisotropy)
    {
        return 0;
    }

    // Else we try to find the best available GPU for our criteria
    switch (deviceProperties.deviceType)
    {
        case vk::PhysicalDeviceType::eVirtualGpu: score += 600; break;
        case vk::PhysicalDeviceType::eIntegratedGpu: score += 800; break;
        case vk::PhysicalDeviceType::eDiscreteGpu: score += 1000; break;
        default: break;
    }

    return score;
}

vk::raii::PhysicalDevice M3VKConstruct::MakePhysicalDevice(const vk::raii::Instance& instance, vk::SurfaceKHR windowSurface, const std::vector<const char *>& deviceExtensions)
{
    std::vector<vk::raii::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();

    if(physicalDevices.empty())
    {
        throw std::runtime_error("Failed to find a Vulkan compatible GPU on this device");
    }

    QueueFamilyIds queueFamilyIds;
    vk::PhysicalDeviceProperties properties;

    vk::PhysicalDevice selected = nullptr;

    int bestScore = 0;
    for(const vk::raii::PhysicalDevice& physicalDevice : physicalDevices)
    {
        QueueFamilyIds localQueueIds;
        vk::PhysicalDeviceProperties localProperties;
        int score = Helper::ScorePhysicalDeviceSuitability(*physicalDevice, windowSurface, deviceExtensions, localProperties, localQueueIds);
        if(score > bestScore)
        {
            bestScore = score;
            selected = *physicalDevice;
            queueFamilyIds = localQueueIds;
            properties = localProperties;
        }
    }

    if(!selected)
    {
        throw std::runtime_error("Failed to find a suitable GPU on this device");
    }

    return vk::raii::PhysicalDevice(instance, selected);
}

vk::raii::Device M3VKConstruct::MakeDevice(const vk::raii::PhysicalDevice& physicalDevice, vk::SurfaceKHR windowSurface, const std::vector<const char*>& deviceExtensions)
{
    // Computed locally rather than read from ApplicationInfo: at this point in construction nothing
    // has populated ApplicationInfo's queue family ids yet, so Surface is taken as a parameter instead.
    QueueFamilyIds queueFamilyIds = QueueFamilyIds::QueryQueueFamilies(physicalDevice, windowSurface);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueIds =
    {
        queueFamilyIds.GraphicsCompute.value(),
        queueFamilyIds.Present.value(),
        queueFamilyIds.Transfer.value()
    };

    float queuePriority = 1.0f;
    for(uint32_t queueId : uniqueQueueIds)
    {
        queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{}
            .setQueueFamilyIndex(queueId)
            .setQueueCount(1)
            .setPQueuePriorities(&queuePriority));
    }

    vk::PhysicalDeviceSynchronization2Features sync2Features = vk::PhysicalDeviceSynchronization2Features()
        .setSynchronization2(vk::True);

    vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3Features = vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT{}
        .setPNext(&sync2Features)
        .setExtendedDynamicState3RasterizationSamples(vk::True)
        .setExtendedDynamicState3ColorBlendEnable(vk::True)
        .setExtendedDynamicState3ColorBlendEquation(vk::True)
        .setExtendedDynamicState3ColorWriteMask(vk::True)
        .setExtendedDynamicState3PolygonMode(vk::True)
        .setExtendedDynamicState3AlphaToCoverageEnable(vk::True)
        .setExtendedDynamicState3DepthClampEnable(vk::True)
        .setExtendedDynamicState3LogicOpEnable(vk::True)
        .setExtendedDynamicState3AlphaToOneEnable(vk::True);

    vk::PhysicalDeviceExtendedDynamicState2FeaturesEXT extendedDynamicState2Features = vk::PhysicalDeviceExtendedDynamicState2FeaturesEXT{}
        .setPNext(&extendedDynamicState3Features)
        .setExtendedDynamicState2(vk::True);

    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures = vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT{}
        .setPNext(&extendedDynamicState2Features)
        .setExtendedDynamicState(vk::True);

    vk::PhysicalDeviceShaderObjectFeaturesEXT shaderObjectFeatures = vk::PhysicalDeviceShaderObjectFeaturesEXT{}
        .setPNext(&extendedDynamicStateFeatures)
        .setShaderObject(vk::True);

    vk::PhysicalDeviceVulkan11Features vulkan11Features = vk::PhysicalDeviceVulkan11Features{}
        .setPNext(&shaderObjectFeatures)
        .setShaderDrawParameters(vk::True);

    // enable bindless support
    vk::PhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = vk::PhysicalDeviceDescriptorIndexingFeatures{}
        .setPNext(&vulkan11Features)
        .setShaderSampledImageArrayNonUniformIndexing(vk::True) // different instance in the same wave can access different textures
        .setShaderStorageImageArrayNonUniformIndexing(vk::True)
        .setShaderStorageBufferArrayNonUniformIndexing(vk::True)
        .setDescriptorBindingSampledImageUpdateAfterBind(vk::True) // descriptor can be updated while bound
        .setDescriptorBindingStorageBufferUpdateAfterBind(vk::True)
        .setDescriptorBindingStorageImageUpdateAfterBind(vk::True)
        .setDescriptorBindingPartiallyBound(vk::True) // slot can be empty
        .setDescriptorBindingVariableDescriptorCount(vk::True) // actual count set at all location
        .setRuntimeDescriptorArray(vk::True); // unsized array in shaders

    vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = vk::PhysicalDeviceDynamicRenderingFeatures{}
        .setPNext(&descriptorIndexingFeatures)
       .setDynamicRendering(vk::True);

    vk::PhysicalDeviceFeatures deviceFeatures = vk::PhysicalDeviceFeatures{}
        .setSampleRateShading(VK_TRUE)
        .setMultiDrawIndirect(VK_TRUE)
        .setDrawIndirectFirstInstance(VK_TRUE)
        .setFillModeNonSolid(VK_TRUE)
        .setSamplerAnisotropy(VK_TRUE);

    vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo{}
        .setPNext(&dynamicRenderingFeatures)
        .setQueueCreateInfos(queueCreateInfos)
        .setPEnabledExtensionNames(deviceExtensions)
        .setPEnabledFeatures(&deviceFeatures);

    vk::raii::Device device(physicalDevice, deviceCreateInfo);
    VkExtManager::InitDevice(device);
    return device;
}
