#include "header/VkHandlers/VkPhysicalDeviceHandler.h"
#include "header/ProjectHelper.h"
#include "header/DebugLayer.h"
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <vector>

bool VkPhysicalDeviceHandler::CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char *>& deviceExtensions) const
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> properties(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, properties.data());

    for(const auto& extension : deviceExtensions)
    {
        bool foundExtension = false;
        for(const VkExtensionProperties& property : properties)
        {
            if(strcmp(extension, property.extensionName) == 0)
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

uint32_t VkPhysicalDeviceHandler::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(_internal, &memoryProperties);

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

int VkPhysicalDeviceHandler::ScoreDeviceSuitability(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface, const std::vector<const char *>& deviceExtensions) const
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

    // support of addtionnal feature (texture compression, 64bit double, multi viewport rendering)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    int score = 0;


    ProjectHelper::QueueFamilyIds ids = ProjectHelper::QueryQueueFamilies(physicalDevice, windowSurface);

    bool areAllRequiredExtensionsSupported = CheckDeviceExtensionSupport(physicalDevice, deviceExtensions);

    ProjectHelper::SwapChainSupportDetails swapChainDetails = ProjectHelper::QuerySwapChainSupportDetail(physicalDevice, windowSurface);

    // Mandatory feature, if any return 0 and this will be the only way to have 0 score meaning there's no suitable GPU
    if(!ProjectHelper::QueueFamilyIds::AreAllQueueAvailable(ids)
        || !areAllRequiredExtensionsSupported
        || !swapChainDetails.CheckSwapChainSupportAdequate())
    {
        return 0;
    }

    // Else we try to find the best available GPU for our criteria
    switch (deviceProperties.deviceType)
    {
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: score += 600; break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: score += 800; break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: score += 1000; break;
        default: break;
    }

    return score;
}

VkPhysicalDeviceHandler::VkPhysicalDeviceHandler(VkInstance instance, VkSurfaceKHR windowSurface, const std::vector<const char *>& deviceExtensions)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkPhysicalDeviceHandler Creation !");
#endif

    _internal = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if(deviceCount == 0)
    {
        throw std::runtime_error("Failed to find a Vulkan compatible GPU on this device");
    }

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

    int bestScore = 0;
    for(const VkPhysicalDevice& physicalDevice : physicalDevices)
    {
        int score = ScoreDeviceSuitability(physicalDevice, windowSurface, deviceExtensions);
        if(score > bestScore)
        {
            bestScore = score;
            _internal = physicalDevice;
        }
    }

    if(_internal == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable GPU on this device");
    }

    QueueFamilyIds = ProjectHelper::QueryQueueFamilies(_internal, windowSurface);
}

VkPhysicalDevice VkPhysicalDeviceHandler::Get() const
{
    return _internal;
}

VkPhysicalDeviceHandler::~VkPhysicalDeviceHandler()
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkPhysicalDeviceHandler Destroyed !");
#endif
}

VkPhysicalDeviceHandler::VkPhysicalDeviceHandler(VkPhysicalDeviceHandler && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkPhysicalDeviceHandler Move Creation !");
#endif

    // Note : Since this is not allocated memory, no need to release the previous one
    _internal = other._internal;
}

VkPhysicalDeviceHandler& VkPhysicalDeviceHandler::operator=(VkPhysicalDeviceHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
    }

    return *this;
}
