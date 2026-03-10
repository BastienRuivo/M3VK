#pragma once

#include "header/VkPhysicalDeviceHandler.h"
#include <vulkan/vulkan_core.h>

class VkDeviceHandler
{
    public:
    VkDeviceHandler(const VkPhysicalDeviceHandler& physicalDeviceHandler, VkSurfaceKHR windowSurface, const std::vector<const char*>& deviceExtensions);
    ~VkDeviceHandler();

    VkDevice Get() const;

    public:

    private:
    VkDevice _internal = VK_NULL_HANDLE;
};
