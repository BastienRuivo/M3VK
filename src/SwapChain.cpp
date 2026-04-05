#include "./header/SwapChain.h"
#include "./header/ProjectHelper.h"
#include "header/MultiFrame.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <vulkan/vulkan_core.h>



void SwapChain::CreateImageViews()
{
    ImageViews.Reserve(Images.size());

    for(int i = 0; i < Images.size(); i++)
    {
        ImageViews.EmplaceBack(_device, Images[i], _imageFormat, 1);
    }
}

VkExtent2D SwapChain::SelectSwapExtents(const Window& window, const VkSurfaceCapabilitiesKHR& Capabilities) const
{
    if(Capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return Capabilities.currentExtent;
    }
    else
    {
        int width, height;
        window.GetFramebufferSize(width, height);

        VkExtent2D extent =
        {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        extent.width = std::clamp(extent.width, Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height);
        return extent;
    }
}

VkPresentModeKHR SwapChain::SelectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const
{
    bool hasTripleBuffering = false;
    // basic vsync, guaranteed and more energy efficient
    bool hasVBlank = false;
    bool hasRelaxedDoubleBuffering = false;
    bool hasImmediateMode = false;

    for(const VkPresentModeKHR& mode : availablePresentModes)
    {
        switch (mode) {
            case VK_PRESENT_MODE_IMMEDIATE_KHR: hasImmediateMode = true; break;
            case VK_PRESENT_MODE_MAILBOX_KHR: hasTripleBuffering = true; break;
            case VK_PRESENT_MODE_FIFO_KHR: hasVBlank = true; break;
            case VK_PRESENT_MODE_FIFO_RELAXED_KHR: hasRelaxedDoubleBuffering = true; break;
            default : break;
        }
    }

    if(hasTripleBuffering)
    {
#ifdef M3VK_VERBOSE_LOG
        DebugLayer::Log(DebugLayer::LogType::INFO, "Swap chain mode : Triple buffering");
#endif
        return VK_PRESENT_MODE_MAILBOX_KHR;
    }
    else if(hasVBlank)
    {
#ifdef M3VK_VERBOSE_LOG
        DebugLayer::Log(DebugLayer::LogType::INFO, "Swap chain mode : VBlank");
#endif
        return VK_PRESENT_MODE_FIFO_KHR;
    }
    else if(hasRelaxedDoubleBuffering)
    {
#ifdef M3VK_VERBOSE_LOG
        DebugLayer::Log(DebugLayer::LogType::INFO, "Swap chain mode : Relaxed VBlank");
#endif
        return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    }
    else if(hasImmediateMode)
    {
#ifdef M3VK_VERBOSE_LOG
        DebugLayer::Log(DebugLayer::LogType::INFO, "Swap chain mode : Immediate");
#endif
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    DebugLayer::Log(DebugLayer::LogType::WARNING, "NO SWAP CHAIN MODE FOUND ! Fallbacked to VBlank");
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkSurfaceFormatKHR SwapChain::SelectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const
{
    for(const VkSurfaceFormatKHR& format : availableFormats)
    {
        if(format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    DebugLayer::Log(DebugLayer::LogType::WARNING, "Can't find best swap chain format");

    return availableFormats[0];
}

SwapChain::SwapChain(const Window& window, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR windowSurface)
: ImageViews(0)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "SwapChain Creation !");
#endif

    _device = device;

    ProjectHelper::SwapChainSupportDetails details = ProjectHelper::QuerySwapChainSupportDetail(physicalDevice, windowSurface);

    VkSurfaceFormatKHR format = SelectSwapSurfaceFormat(details.Formats);
    VkPresentModeKHR presentMode = SelectSwapPresentMode(details.PresentsModes);
    VkExtent2D extents = SelectSwapExtents(window, details.Capabilities);

    // recommended to avoid wait on driver completion
    uint32_t imageCount = details.Capabilities.minImageCount + 1;

    // 0 -> no maximum
    if(details.Capabilities.maxImageCount > 0 && imageCount > details.Capabilities.maxImageCount)
    {
        imageCount = details.Capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = windowSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = format.format;
    createInfo.imageColorSpace = format.colorSpace;
    createInfo.imageExtent = extents;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage =VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    ProjectHelper::QueueFamilyIds queueIds = ProjectHelper::QueryQueueFamilies(physicalDevice, windowSurface);

    uint32_t queueFamilyIndices[] = {queueIds.Graphics.value(), queueIds.Present.value()};

    if(queueIds.Graphics != queueIds.Present)
    {
        // image = multiple queue mode and can be accessed anywhere, but slower
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        // image = one queue mode and should be transferred before being used in another queue, faster but ewh
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    // Apply rotation etc... before screen pres
    createInfo.preTransform = details.Capabilities.currentTransform;
    // If we want to do alpha thingies with other windows, nope for now
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    // Clip pixels obscured by a window in front
    createInfo.clipped = VK_TRUE;
    // When we handle resizing
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if(vkCreateSwapchainKHR(_device, &createInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create swap chain !");
    }

    vkGetSwapchainImagesKHR(_device, _internal, &imageCount, nullptr);
    Images.resize(imageCount);
    vkGetSwapchainImagesKHR(_device, _internal, &imageCount, Images.data());

    _imageFormat = format.format;
    _extent = extents;

    CreateImageViews();
}

SwapChain::~SwapChain()
{
    vkDestroySwapchainKHR(_device, _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "SwapChain Destruction !");
#endif
}
