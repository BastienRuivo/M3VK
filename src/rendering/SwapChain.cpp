#include "rendering/SwapChain.h"
#include "application/ApplicationHelper.h"
#include "application/ApplicationInfo.h"
#include "application/DebugLayer.h"
#include "rendering/GPUImage.h"
#include "rendering/MultiFrame.h"
#include "rendering/QueueFamilyIds.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "application/DebugLayer.h"


vk::Extent2D SwapChain::SelectSwapExtents(const Window& window, const vk::SurfaceCapabilitiesKHR& Capabilities) const
{
    if(Capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return Capabilities.currentExtent;
    }
    else
    {
        int width, height;
        window.GetFramebufferSize(width, height);

        vk::Extent2D extent(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

        extent.width = std::clamp(extent.width, Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height);
        return extent;
    }
}

vk::PresentModeKHR SwapChain::SelectSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) const
{
    bool hasTripleBuffering = false;
    // basic vsync, guaranteed and more energy efficient
    bool hasVBlank = false;
    bool hasRelaxedDoubleBuffering = false;
    bool hasImmediateMode = false;

    for(const vk::PresentModeKHR& mode : availablePresentModes)
    {
        switch (mode) {
            case vk::PresentModeKHR::eImmediate: hasImmediateMode = true; break;
            case vk::PresentModeKHR::eMailbox: hasTripleBuffering = true; break;
            case vk::PresentModeKHR::eFifo: hasVBlank = true; break;
            case vk::PresentModeKHR::eFifoRelaxed: hasRelaxedDoubleBuffering = true; break;
            default : break;
        }
    }

    if(hasTripleBuffering)
    {
#ifdef M3VK_VERBOSE_LOG
        DebugLayer::Log(DebugLayer::LogType::INFO, "Swap chain mode : Triple buffering");
#endif
        return vk::PresentModeKHR::eMailbox;
    }
    else if(hasVBlank)
    {
#ifdef M3VK_VERBOSE_LOG
        DebugLayer::Log(DebugLayer::LogType::INFO, "Swap chain mode : VBlank");
#endif
        return vk::PresentModeKHR::eFifo;
    }
    else if(hasRelaxedDoubleBuffering)
    {
#ifdef M3VK_VERBOSE_LOG
        DebugLayer::Log(DebugLayer::LogType::INFO, "Swap chain mode : Relaxed VBlank");
#endif
        return vk::PresentModeKHR::eFifoRelaxed;
    }
    else if(hasImmediateMode)
    {
#ifdef M3VK_VERBOSE_LOG
        DebugLayer::Log(DebugLayer::LogType::INFO, "Swap chain mode : Immediate");
#endif
        return vk::PresentModeKHR::eImmediate;
    }

    DebugLayer::Log(DebugLayer::LogType::WARNING, "NO SWAP CHAIN MODE FOUND ! Fallbacked to VBlank");
    return vk::PresentModeKHR::eFifo;
}

vk::SurfaceFormatKHR SwapChain::SelectSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const
{
    for(const vk::SurfaceFormatKHR& format : availableFormats)
    {
        if(format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return format;
        }
    }

    DebugLayer::Log(DebugLayer::LogType::WARNING, "Can't find best swap chain format");

    return availableFormats[0];
}

SwapChain::SwapChain(const Window& window, vk::SurfaceKHR windowSurface)
: Images(), _viewHandlers()
{
    vk::Device device = ApplicationInfo::Device();
    ApplicationHelper::SwapChainSupportDetails details = ApplicationHelper::QuerySwapChainSupportDetail(ApplicationInfo::PhysicalDevice(), windowSurface);

    vk::SurfaceFormatKHR format = SelectSwapSurfaceFormat(details.Formats);
    vk::PresentModeKHR presentMode = SelectSwapPresentMode(details.PresentsModes);
    vk::Extent2D extents = SelectSwapExtents(window, details.Capabilities);

    // recommended to avoid wait on driver completion
    uint32_t imageCount = details.Capabilities.minImageCount + 1;

    // 0 -> no maximum
    if(details.Capabilities.maxImageCount > 0 && imageCount > details.Capabilities.maxImageCount)
    {
        imageCount = details.Capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.surface = windowSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = format.format;
    createInfo.imageColorSpace = format.colorSpace;
    createInfo.imageExtent = extents;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;

    _minImageCount = createInfo.minImageCount;

    QueueFamilyIds queueIds = QueueFamilyIds::QueryQueueFamilies(ApplicationInfo::PhysicalDevice(), windowSurface);

    uint32_t queueFamilyIndices[] = {queueIds.GraphicsCompute.value(), queueIds.Present.value()};

    if(queueIds.GraphicsCompute != queueIds.Present)
    {
        // image = multiple queue mode and can be accessed anywhere, but slower
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        // image = one queue mode and should be transferred before being used in another queue, faster but ewh
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    // Apply rotation etc... before screen pres
    createInfo.preTransform = details.Capabilities.currentTransform;
    // If we want to do alpha thingies with other windows, nope for now
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    // Clip pixels obscured by a window in front
    createInfo.clipped = VK_TRUE;
    // When we handle resizing
    createInfo.oldSwapchain = nullptr;

    if(device.createSwapchainKHR(&createInfo, nullptr, &_internal) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create swap chain !");
    }

    std::vector<vk::Image> tempImages = device.getSwapchainImagesKHR(_internal);
    imageCount = static_cast<uint32_t>(tempImages.size());

    Images.Reserve(imageCount);
    _viewHandlers.Reserve(imageCount);
    for(uint32_t i = 0; i < imageCount; i++)
    {
        _viewHandlers.EmplaceBack(tempImages[i], format.format, 1);
        ImageReference image
        {
            .Image = tempImages[i],
            .View = _viewHandlers.Internal(i),
            .Format = format.format,
            .Width = extents.width,
            .Height = extents.height,
            .MipCount = 1,
            .ArrayLayerCount = 1
        };
        Images.EmplaceBack(image);
    }

    _imageFormat = format.format;
    _extent = extents;
}

SwapChain::~SwapChain()
{
    vk::Device device = ApplicationInfo::Device();
    // if the system throw an error, ensure that the current frame is finished before destroying
    device.waitIdle();
    device.destroySwapchainKHR(_internal);
}
