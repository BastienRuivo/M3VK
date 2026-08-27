#pragma once

#include <cstdint>
#include <optional>

#include <vulkan/vulkan.hpp>

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

    static QueueFamilyIds QueryQueueFamilies(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR windowSurface)
    {
        QueueFamilyIds queueIds;

        std::vector<vk::QueueFamilyProperties> families = physicalDevice.getQueueFamilyProperties();

        for(int i = 0; i < families.size(); ++i)
        {
            if(families[i].queueFlags & vk::QueueFlagBits::eGraphics && families[i].queueFlags & vk::QueueFlagBits::eCompute)
            {
                queueIds.GraphicsCompute = i;
            }

            if(families[i].queueFlags & vk::QueueFlagBits::eTransfer)
            {
                queueIds.Transfer = i;
            }

            if(physicalDevice.getSurfaceSupportKHR(i, windowSurface))
            {
                queueIds.Present = i;
            }
        }

        return queueIds;
    }
};
