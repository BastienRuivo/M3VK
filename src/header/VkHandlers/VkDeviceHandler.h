#pragma once

#include "header/VkHandlers/VkPhysicalDeviceHandler.h"
#include <vulkan/vulkan_core.h>

class VkDeviceHandler
{
    public:
    VkDeviceHandler(const VkPhysicalDeviceHandler& physicalDeviceHandler, VkSurfaceKHR windowSurface, const std::vector<const char*>& deviceExtensions);
    ~VkDeviceHandler();

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    VkDeviceHandler(VkDeviceHandler&& other) noexcept;
    VkDeviceHandler& operator=(VkDeviceHandler&& other) noexcept;

    VkDeviceHandler(const VkDeviceHandler&) = delete;
    VkDeviceHandler& operator=(const VkDeviceHandler&) = delete;

    inline VkDevice Get() const
    {
        return _internal;
    }

    public:

    private:
    VkDevice _internal = VK_NULL_HANDLE;
};
