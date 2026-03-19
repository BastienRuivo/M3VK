#include "header/VkHandlers/VkDeviceHandler.h"
#include "header/ProjectHelper.h"
#include "header/DebugLayer.h"
#include "header/VkHandlers/VkPhysicalDeviceHandler.h"
#include <set>
#include <vector>
#include <vulkan/vulkan_core.h>

VkDeviceHandler::VkDeviceHandler(const VkPhysicalDeviceHandler& physicalDeviceHandler, VkSurfaceKHR windowSurface, const std::vector<const char*>& deviceExtensions)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkDeviceHandler Creation !");
#endif

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueIds =
    {
        physicalDeviceHandler.QueueFamilyIds.Graphics.value(),
        physicalDeviceHandler.QueueFamilyIds.Present.value(),
        //QueueFamilyId.Copy.value()
    };

    float queuePriority = 1.0f;
    for(uint32_t queueId : uniqueQueueIds)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueId;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if(DebugLayer::Enabled)
    {
        deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(DebugLayer::ValidationLayer.size());
        deviceCreateInfo.ppEnabledLayerNames = DebugLayer::ValidationLayer.data();
    }
    else
    {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    VkResult deviceCreation = vkCreateDevice(physicalDeviceHandler.Get(), &deviceCreateInfo, nullptr, &_internal);
    if(deviceCreation != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create VK Logical Device !");
    }

    //vkGetDeviceQueue(_device, QueueFamilyId.Graphics.value(), 0, &_graphicsQueue);
    //vkGetDeviceQueue(_device, QueueFamilyId.Present.value(), 0, &_presentQueue);
    // use it
    //vkGetDeviceQueue(_device, queueFamilyId.Copy.value(), 0, &_copyQueue);
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
