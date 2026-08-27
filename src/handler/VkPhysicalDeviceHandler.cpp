#include "handler/VkPhysicalDeviceHandler.h"
#include "application/ApplicationInfo.h"
#include "application/ApplicationHelper.h"
#include "application/DebugLayer.h"
#include "rendering/QueueFamilyIds.h"
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <string.h>

bool VkPhysicalDeviceHandler::CheckDeviceExtensionSupport(vk::PhysicalDevice device, const std::vector<const char *>& deviceExtensions) const
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

uint32_t VkPhysicalDeviceHandler::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
{
    vk::PhysicalDeviceMemoryProperties memoryProperties;
    _internal.getMemoryProperties(&memoryProperties);

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

int VkPhysicalDeviceHandler::ScoreDeviceSuitability(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR windowSurface, const std::vector<const char *>& deviceExtensions, vk::PhysicalDeviceProperties& deviceProperties, QueueFamilyIds& familyIds) const
{
    physicalDevice.getProperties(&deviceProperties);

    // support of addtionnal feature (texture compression, 64bit double, multi viewport rendering)
    vk::PhysicalDeviceFeatures deviceFeatures = physicalDevice.getFeatures();

    int score = 0;


    familyIds = QueueFamilyIds::QueryQueueFamilies(physicalDevice, windowSurface);

    bool areAllRequiredExtensionsSupported = CheckDeviceExtensionSupport(physicalDevice, deviceExtensions);

    ApplicationHelper::SwapChainSupportDetails swapChainDetails = ApplicationHelper::QuerySwapChainSupportDetail(physicalDevice, windowSurface);

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

VkPhysicalDeviceHandler::VkPhysicalDeviceHandler(vk::Instance instance, vk::SurfaceKHR windowSurface, const std::vector<const char *>& deviceExtensions)
{
    _internal = nullptr;
    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();

    if(physicalDevices.empty())
    {
        throw std::runtime_error("Failed to find a Vulkan compatible GPU on this device");
    }

    QueueFamilyIds queueFamilyIds;
    vk::PhysicalDeviceProperties properties;

    int bestScore = 0;
    for(const vk::PhysicalDevice& physicalDevice : physicalDevices)
    {
        QueueFamilyIds localQueueIds;
        vk::PhysicalDeviceProperties localProperties;
        int score = ScoreDeviceSuitability(physicalDevice, windowSurface, deviceExtensions, localProperties, localQueueIds);
        if(score > bestScore)
        {
            bestScore = score;
            _internal = physicalDevice;
            queueFamilyIds = localQueueIds;
            properties = localProperties;
        }
    }

    if(!_internal)
    {
        throw std::runtime_error("Failed to find a suitable GPU on this device");
    }

    ApplicationInfo::Get().SetPhysicalDeviceInformation(_internal, properties, queueFamilyIds);
}

VkPhysicalDeviceHandler::~VkPhysicalDeviceHandler()
{
    _internal = nullptr;
}
