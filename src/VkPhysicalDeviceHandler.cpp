#include "header/VkPhysicalDeviceHandler.h"
#include "header/M3VKHelper.h"
#include "header/VkDebugLayer.h"
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
            if(VkDebugLayer::Enabled)
            {
                VkDebugLayer::Log(VkDebugLayer::LogType::ERROR, (std::string("Extension not supported : ") + std::string(extension)).c_str());
            }
            return false;
        }
    }

    return true;
}

int VkPhysicalDeviceHandler::ScoreDeviceSuitability(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface, const std::vector<const char *>& deviceExtensions) const
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

    // support of addtionnal feature (texture compression, 64bit double, multi viewport rendering)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    int score = 0;


    M3VKHelper::QueueFamilyId ids = M3VKHelper::QueryQueueFamilies(physicalDevice, windowSurface);

    bool areAllRequiredExtensionsSupported = CheckDeviceExtensionSupport(physicalDevice, deviceExtensions);

    M3VKHelper::SwapChainSupportDetails swapChainDetails = M3VKHelper::QuerySwapChainSupportDetail(physicalDevice, windowSurface);

    // Mandatory feature, if any return 0 and this will be the only way to have 0 score meaning there's no suitable GPU
    if(!M3VKHelper::QueueFamilyId::AreAllQueueAvailable(ids)
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
    VkDebugLayer::Log(VkDebugLayer::LogType::CREATE, "VkPhysicalDeviceHandler Creation !");

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
}

VkPhysicalDevice VkPhysicalDeviceHandler::Get() const
{
    return _internal;
}

VkPhysicalDeviceHandler::~VkPhysicalDeviceHandler()
{
    VkDebugLayer::Log(VkDebugLayer::LogType::DESTROY, "VkPhysicalDeviceHandler Destroyed !");
}
