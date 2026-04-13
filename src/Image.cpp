#include "header/Image.h"
#include "header/CommandBuffer.h"
#include "header/DebugLayer.h"
#include "header/GraphicsBuffer.h"
#include "header/ProjectHelper.h"
#include "header/VkHandlers/VkImageViewHandler.h"
#include "header/VkHandlers/VkPhysicalDeviceHandler.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vulkan/vulkan_core.h>


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

CPUImage::CPUImage(const std::string& path, int channelFormat)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "CPUImage Create !");
#endif
    _data = stbi_load(path.c_str(), &_width, &_height, &_channels, channelFormat);

    _channels = channelFormat;


    if(_data == nullptr)
    {
        const char* reason = stbi_failure_reason();
        if (reason)
        {
            DebugLayer::Log(DebugLayer::LogType::ERROR, std::filesystem::current_path().string() + "/" + path + " : " + reason);
            DebugLayer::Log(DebugLayer::LogType::ERROR, std::string("Failed to load image ") +  path + " : " + reason);
        }

        throw std::runtime_error("Failed to load texture image");
    }
}

CPUImage::~CPUImage()
{
    stbi_image_free(_data);
    _width = _height = _channels = 0;
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "GPUImage Destroyed !");
#endif
}

CPUImage::CPUImage(CPUImage&& other) noexcept
{
    _width = other._width;
    _height = other._height;
    _channels = other._channels;
    _data = other._data;

    other._data = nullptr;
    _width = _height = _channels = 0;
}

CPUImage& CPUImage::operator=(CPUImage&& other) noexcept
{
    if(this != &other)
    {
        _width = other._width;
        _height = other._height;
        _channels = other._channels;
        _data = other._data;

        other._data = nullptr;
        _width = _height = _channels = 0;
    }
    return *this;
}

VkFormat CPUImage::GetGPUFormat() const
{
    switch (_channels)
    {
        case STBI_rgb_alpha: return VK_FORMAT_R8G8B8A8_SRGB;
        default: throw std::runtime_error("Unimplemented Color Format");
    }
}

GPUImage::GPUImage(VkDevice device,  const VkPhysicalDeviceHandler & physicalDevice,  const CPUImage& cpuImg, VkCommandPool pool, VkQueue queue)
    : GPUImage(device, physicalDevice,
        cpuImg.Width(), cpuImg.Height(),
        cpuImg.GetGPUFormat(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
{
    CopyCPUtoGPUImage(cpuImg, physicalDevice, pool, queue);
}

GPUImage::GPUImage(VkDevice device,  const VkPhysicalDeviceHandler & physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryFlags)
: GPUImage(device, physicalDevice, width, height, static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1, format, tiling, imageUsageFlags, memoryFlags)
{

}

GPUImage::GPUImage(VkDevice device,  const VkPhysicalDeviceHandler & physicalDevice, uint32_t width, uint32_t height, uint32_t mipCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryFlags)
: GPUImage(device, physicalDevice, width, height, VK_SAMPLE_COUNT_1_BIT, mipCount, format, tiling, imageUsageFlags, memoryFlags)
{

}

GPUImage::GPUImage(VkDevice device,  const VkPhysicalDeviceHandler & physicalDevice, uint32_t width, uint32_t height, VkSampleCountFlagBits msaaSampleCount, uint32_t mipCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryFlags)
: _view(), _device(device), _width(width), _height(height), _format(format), _mipCount(mipCount)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "GPUImage Create !");
#endif

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

void GPUImage::TransitionLayoutCommand(const CommandBuffer& cmdBuffer, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = 0,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, // used to transfer queue ownership if someday I do a copy queue
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = _internal,
        .subresourceRange
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = _mipCount,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
    };

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if(ProjectHelper::HasStencilComponent(_format))
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
        throw std::runtime_error("Unsupported layout transition !");
    }

    cmdBuffer.Barrier(sourceStage, destinationStage, nullptr, 0, nullptr, 0, &barrier, 1);
}

void GPUImage::TransitionLayout(VkCommandPool pool, VkQueue queue, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    CommandBuffer cmdBuffer(_device, pool, queue);
    cmdBuffer.BeginSingleTime();
    {
        TransitionLayoutCommand(cmdBuffer, oldLayout, newLayout);
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
}

void GPUImage::CopyCPUtoGPUImage(const CPUImage & cpuImg, const VkPhysicalDeviceHandler& physicalDevice, VkCommandPool pool, VkQueue queue)
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

void GPUImage::GenerateMipmapsCommand(const CommandBuffer& cmdBuffer, const VkPhysicalDeviceHandler& physicalDevice)
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


GPUImage::~GPUImage()
{
     if(_internal == VK_NULL_HANDLE) return;

    vkDestroyImage(_device, _internal, nullptr);
    vkFreeMemory(_device, _memoryInternal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "GPUImage Destroyed !");
#endif
}
