#pragma once

#include "header/MultiFrame.h"
#include "header/VkHandlers/VkImageViewHandler.h"
#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "header/Window.h"

class SwapChain
{
    public:



    std::vector<VkImage> Images;
    MultiFrameHandler<VkImageViewHandler> ImageViews;
    SwapChain(const Window & window, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR windowSurface);
    ~SwapChain();


    VkSurfaceFormatKHR SelectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    VkPresentModeKHR SelectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    VkExtent2D SelectSwapExtents(const Window& window, const VkSurfaceCapabilitiesKHR& Capabilities) const;

    inline VkFormat GetImageFormat() const { return _imageFormat; }
    inline VkExtent2D GetExtent() const { return _extent; }
    inline VkSwapchainKHR Get() const { return _internal; }


    private:
    void CreateImageViews();

    VkFormat _imageFormat;
    VkExtent2D _extent;
    VkSwapchainKHR _internal;
    VkDevice _device;
};
