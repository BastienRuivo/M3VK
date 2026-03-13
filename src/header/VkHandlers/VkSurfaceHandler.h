#pragma once

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <vulkan/vulkan_core.h>

class VkSurfaceHandler
{
    public:
    VkSurfaceHandler(VkInstance instance, GLFWwindow* pWindow);
    ~VkSurfaceHandler();

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    VkSurfaceHandler(VkSurfaceHandler&& other) noexcept;
    VkSurfaceHandler& operator=(VkSurfaceHandler&& other) noexcept;

    VkSurfaceHandler(const VkSurfaceHandler&) = delete;
    VkSurfaceHandler& operator=(const VkSurfaceHandler&) = delete;

    VkSurfaceKHR Get() const;

    private:
    VkSurfaceKHR _internal = VK_NULL_HANDLE;
    VkInstance _instance = VK_NULL_HANDLE;
};
