#include "rendering/SwapChain.h"
#include "application/ApplicationHelper.h"
#include "application/ApplicationInfo.h"
#include "application/DebugLayer.h"
#include "allocation/RaiiHelper.h"
#include "rendering/GPUImage.h"
#include "rendering/MultiFrame.h"
#include "rendering/QueueFamilyIds.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdint>
#include <limits>
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

vk::raii::SwapchainKHR SwapChain::MakeSwapChainInternal(const Window& window, vk::SurfaceKHR windowSurface)
{
    vk::Device device = ApplicationInfo::Device();
    ApplicationHelper::SwapChainSupportDetails details = ApplicationHelper::QuerySwapChainSupportDetail(ApplicationInfo::PhysicalDevice(), windowSurface);

    vk::SurfaceFormatKHR format = SelectSwapSurfaceFormat(details.Formats);
    vk::PresentModeKHR presentMode = SelectSwapPresentMode(details.PresentsModes);
    _extents = SelectSwapExtents(window, details.Capabilities);
    _imageFormat = format.format;

    // recommended to avoid wait on driver completion
    uint32_t imageCount = details.Capabilities.minImageCount + 1;

    // 0 -> no maximum
    if(details.Capabilities.maxImageCount > 0 && imageCount > details.Capabilities.maxImageCount)
    {
        imageCount = details.Capabilities.maxImageCount;
    }
    _minImageCount = imageCount;

    vk::SwapchainCreateInfoKHR createInfo = vk::SwapchainCreateInfoKHR{}
        .setSurface(windowSurface)
        .setMinImageCount(imageCount)
        .setImageFormat(format.format)
        .setImageColorSpace(format.colorSpace)
        .setImageExtent(_extents)
        .setImageArrayLayers(1u)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
        .setPreTransform(details.Capabilities.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setPresentMode(presentMode)
        .setClipped(vk::True); // Clip pixels obscured by a window in front


    QueueFamilyIds queueIds = QueueFamilyIds::QueryQueueFamilies(ApplicationInfo::PhysicalDevice(), windowSurface);

    uint32_t queueFamilyIndices[] = {queueIds.GraphicsCompute.value(), queueIds.Present.value()};

    if(queueIds.GraphicsCompute != queueIds.Present)
    {
        // image = multiple queue mode and can be accessed anywhere, but slower
        createInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
        createInfo.setQueueFamilyIndexCount(2);
        createInfo.setPQueueFamilyIndices(queueFamilyIndices);
    }
    else
    {
        // image = one queue mode and should be transferred before being used in another queue, faster but ewh
        createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
        createInfo.setQueueFamilyIndexCount(0);
        createInfo.setPQueueFamilyIndices(nullptr);
    }

    return vk::raii::SwapchainKHR(ApplicationInfo::RaiiDevice(), createInfo);
}

SwapChain::SwapChain(const Window& window, vk::SurfaceKHR windowSurface)
: _internal(MakeSwapChainInternal(window, windowSurface)),
Images(), _viewHandlers()
{


    std::vector<vk::Image> tempImages = ApplicationInfo::Device().getSwapchainImagesKHR(_internal);
    uint32_t imageCount = static_cast<uint32_t>(tempImages.size());

    Images.reserve(imageCount);
    _viewHandlers.reserve(imageCount);

    for(uint32_t i = 0; i < imageCount; i++)
    {
        _viewHandlers.emplace_back(RaiiHelper::MakeImageView(tempImages[i], _imageFormat, 1));
        ImageReference image
        {
            .Image = tempImages[i],
            .View = _viewHandlers[i],
            .Format = _imageFormat,
            .Width = _extents.width,
            .Height = _extents.height,
            .MipCount = 1,
            .ArrayLayerCount = 1
        };
        Images.emplace_back(image);
    }
}

SwapChain::~SwapChain()
{
    vk::Device device = ApplicationInfo::Device();
    // if the system throw an error, ensure that the current frame is finished before destroying
    device.waitIdle();
}
