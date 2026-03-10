#pragma once

#include "header/M3VKHelper.h"
#include <vector>
#include <vulkan/vulkan_core.h>

class VkPhysicalDeviceHandler
{
    public:
    VkPhysicalDeviceHandler(VkInstance instance, VkSurfaceKHR windowSurface, const std::vector<const char *>& deviceExtensions);
    ~VkPhysicalDeviceHandler();

    VkPhysicalDevice Get() const;
    int ScoreDeviceSuitability(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface, const std::vector<const char *>& deviceExtensions) const;
    bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice, const std::vector<const char *>& deviceExtensions) const;

    M3VKHelper::QueueFamilyIds QueueFamilyIds;

    private:
    VkPhysicalDevice _internal = VK_NULL_HANDLE;
};
