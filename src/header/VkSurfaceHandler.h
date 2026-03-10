#pragma once

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <vulkan/vulkan_core.h>

class VkSurfaceHandler
{
    public:
    VkSurfaceHandler(VkInstance instance, GLFWwindow* pWindow);
    ~VkSurfaceHandler();

    VkSurfaceKHR Get() const;

    private:
    VkSurfaceKHR _internal = VK_NULL_HANDLE;
    VkInstance _instance = VK_NULL_HANDLE;
};
