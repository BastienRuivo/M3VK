#include "rendering/GPUImage.h"
#include "application/ApplicationHelper.h"
#include "rendering/ImageHelper.h"
#include "application/ApplicationInfo.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GraphicsBuffer.h"
#include "handler/VkImageViewHandler.h"
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vulkan/vulkan_core.h>

GPUImage::GPUImage(uint32_t width, uint32_t height, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags, VkFormat format, VkImageTiling tiling, VkSampleCountFlagBits msaaSampleCount)
: GPUImage(width, height, 1, 0, usageFlags, memoryFlags, format, ImageHelper::GetMipCount(width, height), tiling, msaaSampleCount)
{

}

GPUImage::GPUImage(uint32_t width, uint32_t height, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags, VkFormat format, uint32_t mipCount, VkImageTiling tiling, VkSampleCountFlagBits msaaSampleCount)
: GPUImage(width, height, 1, 0, usageFlags, memoryFlags, format, mipCount, tiling, msaaSampleCount)
{

}

GPUImage::GPUImage(uint32_t width, uint32_t height, uint32_t arrayLayers, VkImageCreateFlags createFlags, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags, VkFormat format, uint32_t mipCount, VkImageTiling tiling, VkSampleCountFlagBits msaaSampleCount)
{
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

    _view = VkImageViewHandler(image, format, mipCount, ApplicationHelper::GetImageAspectFlags(format), createFlags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D);

    _internal =
    {
        .Image = image,
        .View = _view.Internal(),
        .Format = format,
        .Width = static_cast<uint32_t>(width),
        .Height = static_cast<uint32_t>(height),
        .MipCount = mipCount,
        .ArrayLayerCount = arrayLayers,
        .Size = memRequirements.size
    };
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
    vkDestroyImage(ApplicationInfo::Device(), _internal.Image, nullptr);
    vkFreeMemory(ApplicationInfo::Device(), _memoryInternal, nullptr);
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
