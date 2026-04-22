#include "rendering/GPUImage.h"
#include "rendering/ImageHelper.h"
#include "application/ApplicationInfo.h"
#include "asset/CPUImage.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GraphicsBuffer.h"
#include "handler/VkImageViewHandler.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vulkan/vulkan_core.h>


#ifdef M3VK_MEMORYLOG
#include "application/DebugLayer.h"
#endif

/* --- GPU Allocated Image --- */

GPUAllocatedImage::GPUAllocatedImage(const CPUImage& cpuImg, VkCommandPool pool, VkQueue queue)
    : GPUAllocatedImage(cpuImg.Width(),
        cpuImg.Height(),
        cpuImg.GetGPUFormat(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
{
    CopyCPUtoGPUImage(cpuImg, pool, queue);
}

GPUAllocatedImage::GPUAllocatedImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryFlags)
: GPUAllocatedImage(width, height, static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1, format, tiling, imageUsageFlags, memoryFlags)
{

}

GPUAllocatedImage::GPUAllocatedImage(uint32_t width, uint32_t height, uint32_t mipCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryFlags)
: GPUAllocatedImage(width, height, VK_SAMPLE_COUNT_1_BIT, mipCount, format, tiling, imageUsageFlags, memoryFlags)
{

}

GPUAllocatedImage::GPUAllocatedImage(uint32_t width, uint32_t height, VkSampleCountFlagBits msaaSampleCount, uint32_t mipCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryFlags)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "GPUImage Create !");
#endif
    VkImageCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent
        {
            .width = static_cast<uint32_t>(width),
            .height = static_cast<uint32_t>(height),
            .depth = 1
        },
        .mipLevels = mipCount,
        .arrayLayers = 1,
        .samples = msaaSampleCount,
        .tiling = tiling, // Optimal tiling data, if need to write / acces directly to the texture need LINEAR wich is classical row column
        .usage = imageUsageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // only used by the graphics queue
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VkImage image = VK_NULL_HANDLE;

    if(vkCreateImage(ApplicationInfo::Device(), &createInfo, nullptr, &image) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create GPU image !");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(ApplicationInfo::Device(), image, &memRequirements);

    VkMemoryAllocateInfo allocInfo
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = ApplicationInfo::FindMemoryType(memRequirements.memoryTypeBits, memoryFlags)
    };

    if(vkAllocateMemory(ApplicationInfo::Device(), &allocInfo, nullptr, &_memoryInternal) != VK_SUCCESS)
    {
        vkDestroyImage(ApplicationInfo::Device(), image, nullptr);
        throw  std::runtime_error("Can't allocate image memory !");
    }

    if(vkBindImageMemory(ApplicationInfo::Device(), image, _memoryInternal, 0) != VK_SUCCESS)
    {
        vkDestroyImage(ApplicationInfo::Device(), image, nullptr);
        vkFreeMemory(ApplicationInfo::Device(), _memoryInternal, nullptr);
        throw  std::runtime_error("Can't bind image memory !");
    }

    _view = VkImageViewHandler(image, format, mipCount);

    _internal =
    {
        .Image = image,
        .View = _view.Internal(),
        .Format = format,
        .Width = static_cast<uint32_t>(width),
        .Height = static_cast<uint32_t>(height),
        .MipCount = mipCount
    };
}


void GPUAllocatedImage::TransitionLayout(VkCommandPool pool, VkQueue queue, VkImageLayout oldLayout, VkImageLayout newLayout) const
{
    CommandBuffer cmdBuffer(pool, queue);
    cmdBuffer.BeginSingleTime();
    {
        ImageHelper::TransitionLayoutCommand(cmdBuffer, _internal, oldLayout, newLayout);
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
}

void GPUAllocatedImage::CopyCPUtoGPUImage(const CPUImage & cpuImg, VkCommandPool pool, VkQueue queue)
{
    StageBuffer stage(cpuImg.Size());
    stage.CopyToBuffer((void*)cpuImg.Data(), cpuImg.Size());

    TransitionLayout(pool, queue, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CommandBuffer cmdBuffer(pool, queue);
    cmdBuffer.BeginSingleTime();
    {
        ImageHelper::TransitionLayoutCommand(cmdBuffer, _internal, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy region
        {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
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
                .width = static_cast<uint32_t>(cpuImg.Width()),
                .height = static_cast<uint32_t>(cpuImg.Height()),
                .depth = 1
            },
        };
        cmdBuffer.CopyBufferToImage(stage.Internal(), _internal.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &region, 1);

        //TransitionLayoutCommand(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        GenerateMipmapsCommand(cmdBuffer);
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
}

void GPUAllocatedImage::GenerateMipmapsCommand(const CommandBuffer& cmdBuffer) const
{
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(ApplicationInfo::PhysicalDevice(), _internal.Format, &formatProperties);
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
        .image = _internal.Image,
        .subresourceRange
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    int32_t mipWidth = _internal.Width;
    int32_t mipHeight = _internal.Height;

    size_t mipCount = _internal.MipCount;
    for(uint32_t i = 1; i < mipCount; ++i)
    {
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

        cmdBuffer.Blit(_internal.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _internal.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

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


GPUAllocatedImage::~GPUAllocatedImage()
{
    vkDestroyImage(ApplicationInfo::Device(), _internal.Image, nullptr);
    vkFreeMemory(ApplicationInfo::Device(), _memoryInternal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "GPUImage Destroyed !");
#endif
}

GPUAllocatedImage::GPUAllocatedImage(GPUAllocatedImage&& other) noexcept
{
    _internal = std::exchange(other._internal, {});
    _memoryInternal = std::exchange(other._memoryInternal, VK_NULL_HANDLE);
    _view = std::move(other._view);
}

GPUAllocatedImage& GPUAllocatedImage::operator=(GPUAllocatedImage&& other) noexcept
{
    if(this != &other)
    {
        _internal = std::exchange(other._internal, {});
        _memoryInternal = std::exchange(other._memoryInternal, VK_NULL_HANDLE);
        _view = std::move(other._view);
    }
    return *this;
}
