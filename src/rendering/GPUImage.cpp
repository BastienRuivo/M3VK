#include "rendering/GPUImage.h"
#include "application/ApplicationHelper.h"
#include "application/DebugLayer.h"
#include "rendering/ImageHelper.h"
#include "application/ApplicationInfo.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GraphicsBuffer.h"
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vulkan/vulkan_core.h>

GPUImage::GPUImage(uint32_t width, uint32_t height, VkImageUsageFlags usageFlags, VkFormat format, VkImageTiling tiling, VkSampleCountFlagBits msaaSampleCount)
: GPUImage(width, height, 1, 0, usageFlags, format, ImageHelper::GetMipCount(width, height), tiling, msaaSampleCount)
{

}

GPUImage::GPUImage(uint32_t width, uint32_t height, VkImageUsageFlags usageFlags, VkFormat format, uint32_t mipCount, VkImageTiling tiling, VkSampleCountFlagBits msaaSampleCount)
: GPUImage(width, height, 1, 0, usageFlags, format, mipCount, tiling, msaaSampleCount)
{

}

void GPUImage::Resize(uint32_t width, uint32_t height)
{
    if(width != _internal.Width || height != _internal.Height)
    {
        DisposeInternal();
        CreateImageInternal(width, height, _internal.ArrayLayerCount, _internal.CreateFlags, _internal.UsageFlags, _internal.Format, _internal.MipCount, _internal.Tiling, _internal.MsaaSampleCount);
    }

    _internal.Width = width;
    _internal.Height = height;
}

void GPUImage::DisposeInternal()
{
    vkDestroyImageView(ApplicationInfo::Device(), _internal.View, nullptr);
    vkDestroyImage(ApplicationInfo::Device(), _internal.Image, nullptr);
    vkFreeMemory(ApplicationInfo::Device(), _memoryInternal, nullptr);

    _internal.Image = VK_NULL_HANDLE;
    _memoryInternal = VK_NULL_HANDLE;
}

VkImageView CreateImageView(ImageReference& image, VkImageAspectFlags aspectMask, VkImageViewType type)
{
    VkImageViewCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image.Image,
        .viewType = type,
        .format = image.Format,
        .subresourceRange =
        {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = image.MipCount,
            .baseArrayLayer = 0,
            .layerCount = image.ArrayLayerCount
        }
    };

    VkImageView view;
    if(vkCreateImageView(ApplicationInfo::Device(), &createInfo, nullptr, &view) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create swap chain images !");
    }

    return view;
}

void GPUImage::CreateImageInternal(uint32_t width, uint32_t height, uint32_t arrayLayers, VkImageCreateFlags createFlags, VkImageUsageFlags usageFlags, VkFormat format, uint32_t mipCount, VkImageTiling tiling, VkSampleCountFlagBits msaaSampleCount)
{
    VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if(usageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    {
        memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    if(usageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    {
        memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    bool isTransient = usageFlags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    if(isTransient)
    {
        memoryFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
    }

    VkImageCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = createFlags,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent
        {
            .width = static_cast<uint32_t>(width),
            .height = static_cast<uint32_t>(height),
            .depth = 1
        },
        .mipLevels = mipCount,
        .arrayLayers = arrayLayers,
        .samples = msaaSampleCount,
        .tiling = tiling, // Optimal tiling data, if need to write / acces directly to the texture need LINEAR wich is classical row column
        .usage = usageFlags,
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

    uint32_t memoryTypeIndex = ApplicationInfo::FindMemoryType(memRequirements.memoryTypeBits, memoryFlags);

    if(memoryTypeIndex == UINT32_MAX)
    {
        if(isTransient)
        {
            DebugLayer::Log(DebugLayer::LogType::WARNING, "Device can't find Lazily allocated memory, fallbacking to allocated one");
            memoryTypeIndex = ApplicationInfo::FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }

        if(memoryTypeIndex == UINT32_MAX)
        {
            throw std::runtime_error("Can't find replacement for memory type" + std::to_string(memoryFlags));
        }
    }

    VkMemoryAllocateInfo allocInfo
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
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

    _internal = ImageReference {
        .Image = image,
        .CreateFlags = createFlags,
        .UsageFlags = usageFlags,
        .Tiling = tiling,
        .MsaaSampleCount = msaaSampleCount,
        .Format = format,
        .Width = static_cast<uint32_t>(width),
        .Height = static_cast<uint32_t>(height),
        .MipCount = mipCount,
        .ArrayLayerCount = arrayLayers,
        .Size = memRequirements.size,
    };

    _internal.View = CreateImageView(_internal, ApplicationHelper::GetImageAspectFlags(_internal.Format), createFlags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D);
}

GPUImage::GPUImage(uint32_t width, uint32_t height, uint32_t arrayLayers, VkImageCreateFlags createFlags, VkImageUsageFlags usageFlags, VkFormat format, uint32_t mipCount, VkImageTiling tiling, VkSampleCountFlagBits msaaSampleCount)
{
   CreateImageInternal(width, height, arrayLayers, createFlags, usageFlags, format, mipCount, tiling, msaaSampleCount);
}


void GPUImage::TransitionLayout(VkCommandPool pool, VkQueue queue, VkImageLayout oldLayout, VkImageLayout newLayout) const
{
    CommandBuffer cmdBuffer(pool, queue);
    cmdBuffer.BeginSingleTime();
    {
        ImageHelper::TransitionLayoutCommand(cmdBuffer, _internal, oldLayout, newLayout);
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
}

void GPUImage::UploadAndGenerateMip(void* data, uint32_t width, uint32_t height, uint32_t pixelStride, VkCommandPool pool, VkQueue queue)
{
    VkDeviceSize size = width * height * pixelStride;
    StageBuffer stage(size, StageBuffer::Usage::Upload);
    stage.MapAndCopyToBuffer(data, 0, size);

    CommandBuffer cmdBuffer(pool, queue);
    cmdBuffer.BeginSingleTime();
    {
        ImageHelper::CopyToImageCommand(cmdBuffer, _internal, 0, stage.Internal());
        ImageHelper::GenerateMipmapsCommand(cmdBuffer, _internal);
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
}

GPUImage::~GPUImage()
{
    DisposeInternal();
}

GPUImage::GPUImage(GPUImage&& other) noexcept
{
    _internal = std::exchange(other._internal, {});
    _memoryInternal = std::exchange(other._memoryInternal, VK_NULL_HANDLE);
    _view = std::move(other._view);
}

GPUImage& GPUImage::operator=(GPUImage&& other) noexcept
{
    if(this != &other)
    {
        _internal = std::exchange(other._internal, {});
        _memoryInternal = std::exchange(other._memoryInternal, VK_NULL_HANDLE);
        _view = std::move(other._view);
    }
    return *this;
}
