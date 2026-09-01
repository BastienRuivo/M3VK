#pragma once

#include "rendering/GPUImage.h"
#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

#include "application/Window.h"

class SwapChain
{
    public:

    // One entry per swapchain image (not per frame in flight), indexed by the
    // image index handed back from vkAcquireNextImage.
    std::vector<ImageReference> Images;
    SwapChain(const Window & window, vk::SurfaceKHR windowSurface);
    ~SwapChain();

    vk::SurfaceFormatKHR SelectSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const;
    vk::PresentModeKHR SelectSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) const;
    vk::Extent2D SelectSwapExtents(const Window& window, const vk::SurfaceCapabilitiesKHR& Capabilities) const;

    inline vk::Format GetImageFormat() const { return _imageFormat; }
    inline vk::Extent2D GetExtent() const { return _extents; }
    inline vk::SwapchainKHR Internal() const { return _internal; }
    inline vk::ImageView View(uint32_t index) const { return Images[index].View; }
    inline uint32_t MinImageCount() const { return _minImageCount; }

    private:
    std::vector<vk::raii::ImageView> _viewHandlers;
    vk::raii::SwapchainKHR MakeSwapChainInternal(const Window& window, vk::SurfaceKHR windowSurface);

    vk::Format _imageFormat = vk::Format::eUndefined;
    vk::Extent2D _extents;
    uint32_t _minImageCount = 1;
    vk::raii::SwapchainKHR _internal;
};
