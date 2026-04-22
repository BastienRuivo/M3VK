#pragma once

#include <cstdint>
#include <optional>

#include <vulkan/vulkan_core.h>

#include <vector>

struct QueueFamilyIds
{
    public:
    std::optional<uint32_t> Graphics;
    std::optional<uint32_t> Present;
    // std::optional<uint32_t> Copy;

    static bool AreAllQueueAvailable(const QueueFamilyIds& queueIds)
    {
        return queueIds.Graphics.has_value()
            && queueIds.Present.has_value();
            //&& queueIds.Copy.has_value();
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
            if(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                queueIds.Graphics = i;
            }
            // else if(families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
            // {
            //     queueIds.Copy = i;
            // }

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
