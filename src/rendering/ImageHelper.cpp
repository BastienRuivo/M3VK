#include "rendering/ImageHelper.h"
#include "application/ApplicationHelper.h"
#include "application/ApplicationInfo.h"
#include "rendering/GPUImage.h"
#include <cmath>
#include <cstdint>
#include <vulkan/vulkan.hpp>

void ImageHelper::TransitionLayoutCommand(const CommandBuffer& cmdBuffer, const ImageReference& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    TransitionLayoutCommand(cmdBuffer, image, 0, image.MipCount, 0, image.ArrayLayerCount, oldLayout, newLayout);
}

void ImageHelper::TransitionLayoutCommand(const CommandBuffer& cmdBuffer, const ImageReference& image, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    vk::ImageMemoryBarrier2 barrier = TransitionLayoutBarrier(image, mipLevel, mipCount, arrayLayer, arrayLayerCount, oldLayout, newLayout);
    cmdBuffer.Barrier({&barrier, 1});
}

void ImageHelper::StorageImageReadWriteCommand(const CommandBuffer &cmdBuffer, const ImageReference &image, bool isWrite, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, vk::ImageLayout oldLayout)
{
    vk::ImageMemoryBarrier2 barrier = StorageImageReadWriteBarrier(image, isWrite, mipLevel, mipCount, arrayLayer, arrayLayerCount, oldLayout);
    cmdBuffer.Barrier({&barrier, 1});
}

void ImageHelper::StorageImageGeneralToLayoutCommand(const CommandBuffer &cmdBuffer, const ImageReference &image, bool isWrite, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, vk::ImageLayout newLayout)
{
    vk::ImageMemoryBarrier2 barrier = StorageImageGeneralToLayoutBarrier(image, isWrite, mipLevel, mipCount, arrayLayer, arrayLayerCount, newLayout);
    cmdBuffer.Barrier({&barrier, 1});
}

void ImageHelper::CopyToImageCommand(const CommandBuffer &cmdBuffer, const ImageReference &image, uint32_t mipLevel, vk::Buffer srcData)
{
    ImageHelper::TransitionLayoutCommand(cmdBuffer, image, mipLevel, 1, 0, 1, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    vk::BufferImageCopy region = vk::BufferImageCopy{}
        .setBufferOffset(0)
        .setBufferRowLength(0)
        .setBufferImageHeight(0)
        .setImageSubresource(vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, mipLevel, 0, 1})
        .setImageOffset(vk::Offset3D{0, 0, 0})
        .setImageExtent(vk::Extent3D{static_cast<uint32_t>(image.Width), static_cast<uint32_t>(image.Height), 1});
    cmdBuffer.CopyBufferToImage(srcData, image.Image, vk::ImageLayout::eTransferDstOptimal, {&region, 1});
}

void ImageHelper::GenerateMipmapsCommand(const CommandBuffer& cmdBuffer, const ImageReference& image)
{
    // Check if image format supports linear blitting
    vk::FormatProperties formatProperties = ApplicationInfo::PhysicalDevice().getFormatProperties(image.Format);
    if(!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
    {
        // TODO Someday : software mipmapping and storing the mipmaps
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    int32_t mipWidth = image.Width;
    int32_t mipHeight = image.Height;

    size_t mipCount = image.MipCount;
    for(uint32_t i = 1; i < mipCount; ++i)
    {
        ImageHelper::TransitionLayoutCommand(cmdBuffer, image, i - 1, 1, 0, image.ArrayLayerCount, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal);
        ImageHelper::TransitionLayoutCommand(cmdBuffer, image, i, 1, 0, image.ArrayLayerCount, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

        vk::ImageBlit blit = vk::ImageBlit{}
            .setSrcSubresource(vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, i - 1, 0, image.ArrayLayerCount})
            .setSrcOffsets({vk::Offset3D{0, 0, 0}, vk::Offset3D{mipWidth, mipHeight, 1}})
            .setDstSubresource(vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, i, 0, image.ArrayLayerCount})
            .setDstOffsets({vk::Offset3D{0, 0, 0}, vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1}});

        cmdBuffer.Blit(image.Image, vk::ImageLayout::eTransferSrcOptimal, image.Image, vk::ImageLayout::eTransferDstOptimal, {&blit, 1}, vk::Filter::eLinear);

        ImageHelper::TransitionLayoutCommand(cmdBuffer, image, i - 1, 1, 0, image.ArrayLayerCount, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

        if(mipWidth > 1) mipWidth /= 2;
        if(mipHeight > 1) mipHeight /= 2;
    }

    ImageHelper::TransitionLayoutCommand(cmdBuffer, image, mipCount - 1, 1, 0, image.ArrayLayerCount, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
}

uint32_t ImageHelper::GetMipCount(uint32_t width, uint32_t height)
{
    return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}

uint32_t ImageHelper::GetBytePerPixel(vk::Format format)
{
    switch (format)
    {
        case vk::Format::eR8G8B8A8Srgb: return 4;
        case vk::Format::eR8G8B8A8Unorm: return 4;
        default: return 0;
    }
}

void FillSrcLayout(vk::ImageMemoryBarrier2& barrier, vk::ImageLayout srcLayout)
{
    switch(srcLayout)
    {
        case vk::ImageLayout::eUndefined:
        {
            barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
            barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
            break;
        }
        case vk::ImageLayout::eTransferDstOptimal:
        {
            barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
            barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
            break;
        }
        case vk::ImageLayout::eShaderReadOnlyOptimal:
        {
            barrier.srcAccessMask = vk::AccessFlagBits2::eShaderRead;
            barrier.srcStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
            break;
        }
        case vk::ImageLayout::eColorAttachmentOptimal:
        {
            barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
            barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
            break;
        }
        case vk::ImageLayout::eTransferSrcOptimal:
        {
            barrier.srcAccessMask = vk::AccessFlagBits2::eTransferRead;
            barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
            break;
        }
        case vk::ImageLayout::eDepthAttachmentOptimal:  // after rendering, i want to read from the depth buffer to create a HiZ buffer
        {
            barrier.srcAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentWrite;
            barrier.srcStageMask = vk::PipelineStageFlagBits2::eLateFragmentTests | vk::PipelineStageFlagBits2::eColorAttachmentOutput;
            break;
        }
        case vk::ImageLayout::eDepthReadOnlyOptimal:
        {
            barrier.srcAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentRead;
            barrier.srcStageMask = vk::PipelineStageFlagBits2::eFragmentShader | vk::PipelineStageFlagBits2::eComputeShader;
            break;
        }
        case vk::ImageLayout::ePresentSrcKHR:
        {
            barrier.srcAccessMask = vk::AccessFlagBits2::eMemoryRead;
            barrier.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
            break;
        }
        default:
        {
            throw std::runtime_error("Unsupported old layout");
        }
    }
}

void FillDstLayout(vk::ImageMemoryBarrier2& barrier, vk::ImageLayout dstlayout)
{
    switch (dstlayout)
    {
        case vk::ImageLayout::eTransferDstOptimal:
        {
            barrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
            barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
            break;
        }
        case vk::ImageLayout::eShaderReadOnlyOptimal:
        {
            barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
            barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
            break;
        }
        case vk::ImageLayout::eDepthAttachmentOptimal:
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        {
            barrier.dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
            barrier.dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests;
            break;
        }
        case vk::ImageLayout::eColorAttachmentOptimal:
        {
            barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite;
            barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
            break;
        }
        case vk::ImageLayout::ePresentSrcKHR:
        {
            barrier.dstAccessMask = vk::AccessFlagBits2::eMemoryRead;
            barrier.dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
            break;
        }
        case vk::ImageLayout::eTransferSrcOptimal:
        {
            barrier.dstAccessMask = vk::AccessFlagBits2::eTransferRead;
            barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
            break;
        }
        case vk::ImageLayout::eDepthReadOnlyOptimal:
        case vk::ImageLayout::eStencilReadOnlyOptimal:
        case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
        {
            barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
            barrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
            break;
        }

        default:
        {
            throw std::runtime_error("Unsupported new layout");
        }
    }
}

vk::ImageAspectFlags ImageHelper::GetAspect(vk::Format format)
{
    vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor;
    if(format == vk::Format::eD32Sfloat || format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint)
    {
        aspect = vk::ImageAspectFlagBits::eDepth;
        if(ApplicationHelper::HasStencilComponent(format))
        {
            aspect |= vk::ImageAspectFlagBits::eStencil;
        }
    }
    return aspect;
}

vk::ImageMemoryBarrier2 ImageHelper::StorageImageReadWriteBarrier(const ImageReference &image, bool isWrite, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, vk::ImageLayout oldLayout)
{
    vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange{}
        .setAspectMask(GetAspect(image.Format))
        .setBaseMipLevel(mipLevel)
        .setLevelCount(mipCount)
        .setBaseArrayLayer(arrayLayer)
        .setLayerCount(arrayLayerCount);

    vk::ImageMemoryBarrier2 barrier = vk::ImageMemoryBarrier2{}
        .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
        .setSrcAccessMask(vk::AccessFlagBits2::eNone)
        .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setDstAccessMask(isWrite ? vk::AccessFlagBits2::eShaderStorageWrite : vk::AccessFlagBits2::eShaderStorageRead)
        .setOldLayout(oldLayout)
        .setNewLayout(vk::ImageLayout::eGeneral)
        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED) // used to transfer queue ownership if someday I do a copy queue
        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setImage(image.Image)
        .setSubresourceRange(subresourceRange);

    if(oldLayout == vk::ImageLayout::eGeneral)
    {
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
        barrier.srcAccessMask = isWrite ? vk::AccessFlagBits2::eShaderStorageRead : vk::AccessFlagBits2::eShaderStorageWrite;
    }
    else
    {
        FillSrcLayout(barrier, oldLayout);
    }

    return barrier;
}

vk::ImageMemoryBarrier2 ImageHelper::StorageImageGeneralToLayoutBarrier(const ImageReference &image, bool isWrite, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, vk::ImageLayout newLayout)
{
    vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange{}
        .setAspectMask(GetAspect(image.Format))
        .setBaseMipLevel(mipLevel)
        .setLevelCount(mipCount)
        .setBaseArrayLayer(arrayLayer)
        .setLayerCount(arrayLayerCount);

    vk::ImageMemoryBarrier2 barrier = vk::ImageMemoryBarrier2{}
        .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setSrcAccessMask(isWrite ? vk::AccessFlagBits2::eShaderStorageWrite : vk::AccessFlagBits2::eShaderStorageRead)
        .setOldLayout(vk::ImageLayout::eGeneral)
        .setNewLayout(newLayout)
        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED) // used to transfer queue ownership if someday I do a copy queue
        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setImage(image.Image)
        .setSubresourceRange(subresourceRange);

    FillDstLayout(barrier, newLayout);

    return barrier;
}

vk::ImageMemoryBarrier2 ImageHelper::TransitionLayoutBarrier(const ImageReference &image, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange{}
        .setAspectMask(GetAspect(image.Format))
        .setBaseMipLevel(mipLevel)
        .setLevelCount(mipCount)
        .setBaseArrayLayer(arrayLayer)
        .setLayerCount(arrayLayerCount);

    vk::ImageMemoryBarrier2 barrier = vk::ImageMemoryBarrier2{}
        .setSrcAccessMask(vk::AccessFlagBits2::eNone)
        .setDstAccessMask(vk::AccessFlagBits2::eNone)
        .setOldLayout(oldLayout)
        .setNewLayout(newLayout)
        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED) // used to transfer queue ownership if someday I do a copy queue
        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setImage(image.Image)
        .setSubresourceRange(subresourceRange);

    FillSrcLayout(barrier, oldLayout);
    FillDstLayout(barrier, newLayout);

    return barrier;
}

vk::ImageView ImageHelper::CreateImageView(ImageReference& image, vk::ImageAspectFlags aspectMask, vk::ImageViewType type)
{
    return CreateImageView(image, aspectMask, type, 0, image.MipCount, 0, type == vk::ImageViewType::eCube ? 6u : 1u);
}

vk::ImageView ImageHelper::CreateImageView(ImageReference& image, vk::ImageAspectFlags aspectMask, vk::ImageViewType type, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount)
{
    vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange{}
        .setAspectMask(aspectMask)
        .setBaseMipLevel(mipLevel)
        .setLevelCount(mipCount)
        .setBaseArrayLayer(arrayLayer)
        .setLayerCount(arrayLayerCount);

    vk::ImageViewCreateInfo createInfo = vk::ImageViewCreateInfo{}
        .setImage(image.Image)
        .setViewType(type)
        .setFormat(image.Format)
        .setSubresourceRange(subresourceRange);

    vk::ImageView view;
    if(ApplicationInfo::Device().createImageView(&createInfo, nullptr, &view) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create swap chain images !");
    }

    return view;
}
