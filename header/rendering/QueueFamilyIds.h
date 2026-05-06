#pragma once

#include <cstdint>
#include <optional>

#include <vulkan/vulkan_core.h>

#include <vector>

struct QueueFamilyIds
{
    public:
    std::optional<uint32_t> GraphicsCompute;
    std::optional<uint32_t> Present;
    std::optional<uint32_t> Transfer; // fallback to Graphics if not available

    static bool AreAllQueueAvailable(const QueueFamilyIds& queueIds)
    {
        return queueIds.GraphicsCompute.has_value()
            && queueIds.Present.has_value()
            && queueIds.Transfer.has_value();
    }

    static QueueFamilyIds QueryQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface)
    {
        QueueFamilyIds queueIds;

        uint32_t queueFamiliesCount = 0;

        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(queueFamiliesCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, families.data());

        for(int i = 0; i < families.size(); ++i)
        {
            if(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                queueIds.GraphicsCompute = i;
            }

            if(families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                queueIds.Transfer = i;
            }
            VkBool32 isPresentSupported = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, windowSurface, &isPresentSupported);

            if(isPresentSupported)
            {
                queueIds.Present = i;
            }
        }

        return queueIds;
    }
};
