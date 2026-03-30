#include "header/Image.h"
#include "header/CommandBuffer.h"
#include "header/DebugLayer.h"
#include "header/GraphicsBuffer.h"
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
        DebugLayer::Log(DebugLayer::LogType::ERROR, std::string("Failed to load image ") +  path);
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
    : GPUImage(device, physicalDevice, cpuImg.GetGPUFormat(), cpuImg.Width(), cpuImg.Height())
{
    CopyCPUtoGPUImage(cpuImg, physicalDevice, pool, queue);
}

GPUImage::GPUImage(VkDevice device,  const VkPhysicalDeviceHandler & physicalDevice,  VkFormat format, uint32_t width, uint32_t height)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "GPUImage Create !");
#endif
    _device = device;
    _format = format;

    VkImageCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.extent.width = static_cast<uint32_t>(width);
    createInfo.extent.height = static_cast<uint32_t>(height);
    createInfo.extent.depth = 1;
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;

    createInfo.format = _format;
    // Optimal tiling data, if need to write / acces directly to the texture need LINEAR wich is classical row column
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // Destination to copy the CPU image via stage buffer & will be used for sampling
    createInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    // only used by the graphics queue
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // Search on this later ?
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if(vkCreateImage(_device, &createInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create GPU image !");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, _internal, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = physicalDevice.FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if(vkAllocateMemory(_device, &allocInfo, nullptr, &_memoryInternal) != VK_SUCCESS)
    {
        throw  std::runtime_error("Can't allocate image memory !");
    }

    vkBindImageMemory(_device, _internal, _memoryInternal, 0);
}

void GPUImage::TransitionImageLayout(VkCommandPool pool, VkQueue queue, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    CommandBuffer cmdBuffer(_device, pool, queue);
    cmdBuffer.BeginSingleTime();
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        // used to transfer queue ownership if someday I do a copy queue
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = _internal;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

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

        cmdBuffer.Barrier(sourceStage, destinationStage, nullptr, 0, nullptr, 0, &barrier, 1);
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
}

void GPUImage::CopyCPUtoGPUImage(const CPUImage & cpuImg, const VkPhysicalDeviceHandler& physicalDevice, VkCommandPool pool, VkQueue queue)
{
    StageBuffer stage(physicalDevice, _device, cpuImg.Size());
    stage.CopyToBuffer(_device, (void*)cpuImg.Data(), cpuImg.Size());
    // can probably be merged
    TransitionImageLayout(pool, queue, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CommandBuffer cmdBuffer(_device, pool, queue);
    cmdBuffer.Begin();
    {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            static_cast<uint32_t>(cpuImg.Width()),
            static_cast<uint32_t>(cpuImg.Height()),
            1
        };
        cmdBuffer.CopyBufferToImage(stage.Get(), _internal, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &region, 1);
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
    TransitionImageLayout(pool, queue, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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
