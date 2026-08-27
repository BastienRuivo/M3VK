#include "rendering/GPUImage.h"
#include "application/ApplicationHelper.h"
#include "rendering/ImageHelper.h"
#include "application/ApplicationInfo.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GraphicsBuffer.h"
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vulkan/vulkan.hpp>

GPUImage::GPUImage(uint32_t width, uint32_t height, vk::ImageUsageFlags usageFlags, vk::Format format, vk::ImageTiling tiling, vk::SampleCountFlagBits msaaSampleCount)
: GPUImage(width, height, 1, {}, usageFlags, format, ImageHelper::GetMipCount(width, height), tiling, msaaSampleCount)
{

}

GPUImage::GPUImage(uint32_t width, uint32_t height, vk::ImageUsageFlags usageFlags, vk::Format format, uint32_t mipCount, vk::ImageTiling tiling, vk::SampleCountFlagBits msaaSampleCount)
: GPUImage(width, height, 1, {}, usageFlags, format, mipCount, tiling, msaaSampleCount)
{

}

bool GPUImage::Resize(uint32_t width, uint32_t height)
{
    if(width != _internal.Width || height != _internal.Height)
    {
        DisposeInternal();
        if(_internal.MipCount > 1)
        {
            _internal.MipCount = ImageHelper::GetMipCount(width, height);
        }
        CreateImageInternal(width, height, _internal.ArrayLayerCount, _internal.CreateFlags, _internal.UsageFlags, _internal.Format, _internal.MipCount, _internal.Tiling, _internal.MsaaSampleCount);
        _internal.Width = width;
        _internal.Height = height;
        return true;
    }

    return false;
}

void GPUImage::DisposeInternal()
{
    vk::Device device = ApplicationInfo::Device();
    device.destroyImageView(_internal.View);
    device.destroyImage(_internal.Image);
    device.freeMemory(_memoryInternal.Memory);

    ApplicationInfo::VRAMRelease(_memoryInternal.Size, ApplicationInfo::AllocType::Image);

    _internal.Image = nullptr;
    _internal.View = nullptr;
    _memoryInternal = {};
}

void GPUImage::CreateImageInternal(uint32_t width, uint32_t height, uint32_t arrayLayers, vk::ImageCreateFlags createFlags, vk::ImageUsageFlags usageFlags, vk::Format format, uint32_t mipCount, vk::ImageTiling tiling, vk::SampleCountFlagBits msaaSampleCount)
{
    vk::Device device = ApplicationInfo::Device();
    vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;

    vk::ImageCreateInfo createInfo{};
    createInfo.flags = createFlags;
    createInfo.imageType = vk::ImageType::e2D;
    createInfo.format = format;
    createInfo.extent = vk::Extent3D(width, height, 1);
    createInfo.mipLevels = mipCount;
    createInfo.arrayLayers = arrayLayers;
    createInfo.samples = msaaSampleCount;
    createInfo.tiling = tiling; // Optimal tiling data, if need to write / acces directly to the texture need LINEAR wich is classical row column
    createInfo.usage = usageFlags;
    createInfo.sharingMode = vk::SharingMode::eExclusive; // only used by the graphics queue
    createInfo.initialLayout = vk::ImageLayout::eUndefined;

    vk::Image image;
    if(device.createImage(&createInfo, nullptr, &image) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create GPU image !");
    }

    vk::MemoryRequirements memRequirements = device.getImageMemoryRequirements(image);

    uint32_t memoryTypeIndex = ApplicationInfo::FindMemoryType(memRequirements.memoryTypeBits, memoryFlags);

    if(memoryTypeIndex == UINT32_MAX)
    {
        device.destroyImage(image);
        throw std::runtime_error("Can't find replacement for memory type" + vk::to_string(memoryFlags));
    }

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    vk::Result memoryResult = device.allocateMemory(&allocInfo, nullptr, &_memoryInternal.Memory);
    if(memoryResult != vk::Result::eSuccess)
    {
        device.destroyImage(image);
        throw std::runtime_error("Can't allocate image memory !");
    }
    _memoryInternal.Size = allocInfo.allocationSize;
    ApplicationInfo::VRAMAllocate(_memoryInternal.Size, ApplicationInfo::AllocType::Image);

    try
    {
        // bindImageMemory has no Result-returning overload in enhanced mode; it throws on failure
        device.bindImageMemory(image, _memoryInternal.Memory, 0);
    }
    catch (const vk::SystemError&)
    {
        device.destroyImage(image);
        device.freeMemory(_memoryInternal.Memory);
        ApplicationInfo::VRAMRelease(_memoryInternal.Size, ApplicationInfo::AllocType::Image);
        throw std::runtime_error("Can't bind image memory !");
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

    _internal.View = ImageHelper::CreateImageView(_internal, ApplicationHelper::GetImageAspectFlags(_internal.Format), (createFlags & vk::ImageCreateFlagBits::eCubeCompatible) ? vk::ImageViewType::eCube : vk::ImageViewType::e2D);
}

GPUImage::GPUImage(uint32_t width, uint32_t height, uint32_t arrayLayers, vk::ImageCreateFlags createFlags, vk::ImageUsageFlags usageFlags, vk::Format format, uint32_t mipCount, vk::ImageTiling tiling, vk::SampleCountFlagBits msaaSampleCount)
{
   CreateImageInternal(width, height, arrayLayers, createFlags, usageFlags, format, mipCount, tiling, msaaSampleCount);
}


void GPUImage::TransitionLayout(vk::CommandPool pool, vk::Queue queue, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) const
{
    CommandBuffer cmdBuffer(pool, queue);
    cmdBuffer.BeginSingleTime();
    {
        ImageHelper::TransitionLayoutCommand(cmdBuffer, _internal, oldLayout, newLayout);
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
}

void GPUImage::UploadAndGenerateMip(void* data, uint32_t width, uint32_t height, uint32_t pixelStride, vk::CommandPool pool, vk::Queue queue)
{
    vk::DeviceSize size = width * height * pixelStride;
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
    _memoryInternal = std::exchange(other._memoryInternal, {});
}

GPUImage& GPUImage::operator=(GPUImage&& other) noexcept
{
    if(this != &other)
    {
        _internal = std::exchange(other._internal, {});
        _memoryInternal = std::exchange(other._memoryInternal, {});
    }
    return *this;
}
