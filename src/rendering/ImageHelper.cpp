#include "rendering/ImageHelper.h"
#include "application/ApplicationInfo.h"
#include "rendering/GPUImage.h"
#include <cstdint>

void ImageHelper::TransitionLayoutCommand(const CommandBuffer& cmdBuffer, const ImageReference& image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    cmdBuffer.TransitionImageLayout(image.Image, image.Format, 0, image.MipCount, oldLayout, newLayout);
}

void ImageHelper::TransitionLayoutCommand(const CommandBuffer& cmdBuffer, const ImageReference& image, uint32_t mipLevel, uint32_t mipCount, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    cmdBuffer.TransitionImageLayout(image.Image, image.Format, mipLevel, mipCount, oldLayout, newLayout);
}

void ImageHelper::CopyToImageCommand(const CommandBuffer &cmdBuffer, const ImageReference &image, uint32_t mipLevel, VkBuffer srcData, uint32_t width, uint32_t height)
{
    ImageHelper::TransitionLayoutCommand(cmdBuffer, image, mipLevel, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region
    {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = mipLevel,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .imageOffset
        {
            .x = 0,
            .y = 0,
            .z = 0
        },
        .imageExtent
        {
            .width = static_cast<uint32_t>(width),
            .height = static_cast<uint32_t>(height),
            .depth = 1
        },
    };
    cmdBuffer.CopyBufferToImage(srcData, image.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &region, 1);
}

void ImageHelper::GenerateMipmapsCommand(const CommandBuffer& cmdBuffer, const ImageReference& image)
{
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(ApplicationInfo::PhysicalDevice(), image.Format, &formatProperties);
    if(!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        // TODO Someday : software mipmapping and storing the mipmaps
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkImageMemoryBarrier barrier
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image.Image,
        .subresourceRange
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    int32_t mipWidth = image.Width;
    int32_t mipHeight = image.Height;

    size_t mipCount = image.MipCount;
    for(uint32_t i = 1; i < mipCount; ++i)
    {
        ImageHelper::TransitionLayoutCommand(cmdBuffer, image, i, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        cmdBuffer.Barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, nullptr, 0, nullptr, 0, &barrier, 1);

        VkImageBlit blit
        {
            .srcSubresource
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i - 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .srcOffsets =
            {
                {0, 0, 0},
                {mipWidth, mipHeight, 1}
            },
            .dstSubresource
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .dstOffsets =
            {
                {0, 0, 0},
                {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1}
            },

        };

        cmdBuffer.Blit(image.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        cmdBuffer.Barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, nullptr, 0, nullptr, 0, &barrier, 1);

        if(mipWidth > 1) mipWidth /= 2;
        if(mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipCount - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    cmdBuffer.Barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, nullptr, 0, nullptr, 0, &barrier, 1);
}
