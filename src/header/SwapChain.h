#pragma once

#include "header/GPUImage.h"
#include "header/MultiFrame.h"
#include "header/Handlers/VkImageViewHandler.h"
#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>

#include "header/Window.h"

class SwapChain
{
    public:

    MultiFrameHandler<ImageReference> Images;
    SwapChain(const Window & window, VkSurfaceKHR windowSurface);
    ~SwapChain();

    VkSurfaceFormatKHR SelectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    VkPresentModeKHR SelectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    VkExtent2D SelectSwapExtents(const Window& window, const VkSurfaceCapabilitiesKHR& Capabilities) const;

    inline VkFormat GetImageFormat() const { return _imageFormat; }
    inline VkExtent2D GetExtent() const { return _extent; }
    inline VkSwapchainKHR Internal() const { return _internal; }
    inline VkImageView View(uint32_t index) const { return Images.Get(index).View; }

    private:
    MultiFrameHandler<VkImageViewHandler> _viewHandlers;

    VkFormat _imageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D _extent;
    VkSwapchainKHR _internal = VK_NULL_HANDLE;
};
