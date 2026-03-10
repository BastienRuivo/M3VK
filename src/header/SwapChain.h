#pragma once

#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "header/Window.h"

class SwapChain
{
    public:
    VkFormat ImageFormat;
    VkExtent2D Extent;
    VkSwapchainKHR _internal;
    VkDevice _device;

    std::vector<VkImage> Images;
    std::vector<VkImageView> ImageViews;
    SwapChain(const Window & window, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR windowSurface);
    ~SwapChain();


    VkSurfaceFormatKHR SelectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    VkPresentModeKHR SelectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    VkExtent2D SelectSwapExtents(const Window& window, const VkSurfaceCapabilitiesKHR& Capabilities) const;


    private:
    void CreateImageView();
};
