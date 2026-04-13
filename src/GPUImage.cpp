#include "header/GPUImage.h"
#include "header/CPUImage.h"
#include "header/CommandBuffer.h"
#include "header/GraphicsBuffer.h"
#include "header/VkHandlers/VkImageViewHandler.h"
#include "header/VkHandlers/VkPhysicalDeviceHandler.h"
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>


#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

void GPUImage::TransitionLayoutCommand(const CommandBuffer& cmdBuffer, VkImageLayout oldLayout, VkImageLayout newLayout) const
{
    cmdBuffer.TransitionImageLayout(_internal, _format, _mipCount, oldLayout, newLayout);
}

/* --- GPU Allocated Image --- */

GPUAllocatedImage::GPUAllocatedImage(VkDevice device,  const VkPhysicalDeviceHandler & physicalDevice,  const CPUImage& cpuImg, VkCommandPool pool, VkQueue queue)
    : GPUAllocatedImage(device, physicalDevice,
        cpuImg.Width(), cpuImg.Height(),
        cpuImg.GetGPUFormat(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
{
    CopyCPUtoGPUImage(cpuImg, physicalDevice, pool, queue);
}

GPUAllocatedImage::GPUAllocatedImage(VkDevice device,  const VkPhysicalDeviceHandler & physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryFlags)
: GPUAllocatedImage(device, physicalDevice, width, height, static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1, format, tiling, imageUsageFlags, memoryFlags)
{

}

GPUAllocatedImage::GPUAllocatedImage(VkDevice device,  const VkPhysicalDeviceHandler & physicalDevice, uint32_t width, uint32_t height, uint32_t mipCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryFlags)
: GPUAllocatedImage(device, physicalDevice, width, height, VK_SAMPLE_COUNT_1_BIT, mipCount, format, tiling, imageUsageFlags, memoryFlags)
{

}

GPUAllocatedImage::GPUAllocatedImage(VkDevice device,  const VkPhysicalDeviceHandler & physicalDevice, uint32_t width, uint32_t height, VkSampleCountFlagBits msaaSampleCount, uint32_t mipCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryFlags)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "GPUImage Create !");
#endif
    _device = device;
    _width = width;
    _height = height;
    _format = format;
    _mipCount = mipCount;

    VkImageCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = _format,
        .extent
        {
            .width = static_cast<uint32_t>(width),
            .height = static_cast<uint32_t>(height),
            .depth = 1
        },
        .mipLevels = _mipCount,
        .arrayLayers = 1,
        .samples = msaaSampleCount,
        .tiling = tiling, // Optimal tiling data, if need to write / acces directly to the texture need LINEAR wich is classical row column
        .usage = imageUsageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // only used by the graphics queue
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    if(vkCreateImage(_device, &createInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create GPU image !");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, _internal, &memRequirements);

    VkMemoryAllocateInfo allocInfo
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = physicalDevice.FindMemoryType(memRequirements.memoryTypeBits, memoryFlags)
    };

    if(vkAllocateMemory(_device, &allocInfo, nullptr, &_memoryInternal) != VK_SUCCESS)
    {
        vkDestroyImage(_device, _internal, nullptr);
        throw  std::runtime_error("Can't allocate image memory !");
    }

    if(vkBindImageMemory(_device, _internal, _memoryInternal, 0) != VK_SUCCESS)
    {
        vkDestroyImage(_device, _internal, nullptr);
        vkFreeMemory(_device, _memoryInternal, nullptr);
        throw  std::runtime_error("Can't bind image memory !");
    }

    _view = VkImageViewHandler(_device, _internal, _format, _mipCount);
}


void GPUAllocatedImage::TransitionLayout(VkCommandPool pool, VkQueue queue, VkImageLayout oldLayout, VkImageLayout newLayout) const
{
    CommandBuffer cmdBuffer(_device, pool, queue);
    cmdBuffer.BeginSingleTime();
    {
        TransitionLayoutCommand(cmdBuffer, oldLayout, newLayout);
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
}

void GPUAllocatedImage::CopyCPUtoGPUImage(const CPUImage & cpuImg, const VkPhysicalDeviceHandler& physicalDevice, VkCommandPool pool, VkQueue queue)
{
    StageBuffer stage(physicalDevice, _device, cpuImg.Size());
    stage.CopyToBuffer(_device, (void*)cpuImg.Data(), cpuImg.Size());

    TransitionLayout(pool, queue, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CommandBuffer cmdBuffer(_device, pool, queue);
    cmdBuffer.BeginSingleTime();
    {
        TransitionLayoutCommand(cmdBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

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
        cmdBuffer.CopyBufferToImage(stage.Get(), _internal, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &region, 1);

        //TransitionLayoutCommand(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        GenerateMipmapsCommand(cmdBuffer, physicalDevice);
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
}

void GPUAllocatedImage::GenerateMipmapsCommand(const CommandBuffer& cmdBuffer, const VkPhysicalDeviceHandler& physicalDevice) const
{
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice.Get(), _format, &formatProperties);
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
        .image = _internal,
        .subresourceRange
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    int32_t mipWidth = _width;
    int32_t mipHeight = _height;

    for(uint32_t i = 1; i < _mipCount; ++i)
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

        cmdBuffer.Blit(_internal, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _internal, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        cmdBuffer.Barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, nullptr, 0, nullptr, 0, &barrier, 1);

        if(mipWidth > 1) mipWidth /= 2;
        if(mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = _mipCount - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    cmdBuffer.Barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, nullptr, 0, nullptr, 0, &barrier, 1);
}


GPUAllocatedImage::~GPUAllocatedImage()
{
     if(_internal == VK_NULL_HANDLE) return;

    vkDestroyImage(_device, _internal, nullptr);
    vkFreeMemory(_device, _memoryInternal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "GPUImage Destroyed !");
#endif
}

GPUAllocatedImage::GPUAllocatedImage(GPUAllocatedImage&& other) noexcept
{
    _internal = other._internal;
    _memoryInternal = other._memoryInternal;
    _device = other._device;
    _format = other._format;
    _width = other._width;
    _height = other._height;
    _mipCount = other._mipCount;
    _view = std::move(other._view);

    other._internal = VK_NULL_HANDLE;
    other._memoryInternal = VK_NULL_HANDLE;
}

GPUAllocatedImage& GPUAllocatedImage::operator=(GPUAllocatedImage&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        _memoryInternal = other._memoryInternal;
        _device = other._device;
        _format = other._format;
        _width = other._width;
        _height = other._height;
        _mipCount = other._mipCount;
        _view = std::move(other._view);

        other._internal = VK_NULL_HANDLE;
        other._memoryInternal = VK_NULL_HANDLE;
    }
    return *this;
}
