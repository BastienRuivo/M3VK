#pragma once

#include "header/GPUImage.h"
#include "header/MultiFrame.h"
#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "header/Window.h"

class SwapChain
{
    public:
    class SwapChainImage : public GPUImage
    {
        public:
        SwapChainImage(VkDevice device, VkImage swapChainImage, VkFormat format, uint32_t width, uint32_t height);

        ~SwapChainImage() override = default;

        SwapChainImage(const SwapChainImage&)            = delete;
        SwapChainImage& operator=(const SwapChainImage&) = delete;

        SwapChainImage(SwapChainImage&&) noexcept;
        SwapChainImage& operator=(SwapChainImage&&) noexcept;
    };

    MultiFrameHandler<SwapChainImage> Images;
    SwapChain(const Window & window, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR windowSurface);
    ~SwapChain();

    VkSurfaceFormatKHR SelectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    VkPresentModeKHR SelectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    VkExtent2D SelectSwapExtents(const Window& window, const VkSurfaceCapabilitiesKHR& Capabilities) const;

    inline VkFormat GetImageFormat() const { return _imageFormat; }
    inline VkExtent2D GetExtent() const { return _extent; }
    inline VkSwapchainKHR Get() const { return _internal; }

    inline VkImageView GetView(uint32_t index) const { return Images.Get(index).GetView(); }


    private:

    VkFormat _imageFormat;
    VkExtent2D _extent;
    VkSwapchainKHR _internal = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
};
