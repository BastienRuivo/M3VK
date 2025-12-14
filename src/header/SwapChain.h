#ifndef SWAP_CHAIN_CLASS
#define SWAP_CHAIN_CLASS

#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//#include "VkDebugLayer.h"
#include "header/Window.h"

class SwapChain
{
    public:
    VkFormat ImageFormat;
    VkExtent2D Extent;
    VkSwapchainKHR Internal;

    std::vector<VkImage> Images;
    std::vector<VkImageView> ImageViews;
    void Create(const Window & window, const VkPhysicalDevice& physicalDevice, const VkDevice& logicalDevice, const VkSurfaceKHR& windowSurface);
    void Dipose(const VkDevice& logicalDevice);


    VkSurfaceFormatKHR SelectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    VkPresentModeKHR SelectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    VkExtent2D SelectSwapExtents(const Window& window, const VkSurfaceCapabilitiesKHR& Capabilities) const;


    private:
    void CreateImageView(const VkDevice& logicalDevice);
};

#endif
