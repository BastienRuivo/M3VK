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

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    VkPhysicalDeviceHandler(VkPhysicalDeviceHandler&& other) noexcept;
    VkPhysicalDeviceHandler& operator=(VkPhysicalDeviceHandler&& other) noexcept;

    VkPhysicalDeviceHandler(const VkPhysicalDeviceHandler&) = delete;
    VkPhysicalDeviceHandler& operator=(const VkPhysicalDeviceHandler&) = delete;

    ProjectHelper::QueueFamilyIds QueueFamilyIds;

    private:
    VkPhysicalDevice _internal = VK_NULL_HANDLE;
};
