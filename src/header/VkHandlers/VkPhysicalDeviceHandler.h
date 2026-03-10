#pragma once

#include "header/ProjectHelper.h"
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

    ProjectHelper::QueueFamilyIds QueueFamilyIds;

    private:
    VkPhysicalDevice _internal = VK_NULL_HANDLE;
};
