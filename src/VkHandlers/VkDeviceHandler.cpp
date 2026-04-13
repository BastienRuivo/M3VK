#include "header/VkHandlers/VkDeviceHandler.h"
#include "header/ApplicationInfo.h"
#include "header/DebugLayer.h"
#include "header/VkHandlers/VkPhysicalDeviceHandler.h"
#include <set>
#include <vector>
#include <vulkan/vulkan_core.h>

VkDeviceHandler::VkDeviceHandler(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface, const std::vector<const char*>& deviceExtensions)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkDeviceHandler Creation !");
#endif

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueIds =
    {
        ApplicationInfo::Get().GetGraphicsQueueId(),
        ApplicationInfo::Get().GetPresentQueueId(),
        //QueueFamilyId.Copy.value()
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

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .dynamicRendering = VK_TRUE
    };

    VkPhysicalDeviceFeatures deviceFeatures
    {
        .sampleRateShading = VK_TRUE,
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

    VkResult deviceCreation = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &_internal);
    if(deviceCreation != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create VK Logical Device !");
    }
}

VkDeviceHandler::~VkDeviceHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroyDevice(_internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkDeviceHandler Destroyed !");
#endif
}

VkDeviceHandler::VkDeviceHandler(VkDeviceHandler && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkDeviceHandler Move Creation !");
#endif

    _internal = other._internal;
    other._internal = VK_NULL_HANDLE;
}

VkDeviceHandler& VkDeviceHandler::operator=(VkDeviceHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        other._internal = VK_NULL_HANDLE;
    }

    return *this;
}
