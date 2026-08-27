#pragma once

#include "allocation/RessourceUsage.h"
#include <cstddef>
#include <cstdint>
#include <stb_image.h>
#include <vulkan/vulkan.hpp>

struct ImageReference
{
    vk::Image Image;
    vk::ImageCreateFlags CreateFlags{};
    vk::ImageUsageFlags UsageFlags{};
    vk::ImageTiling Tiling = vk::ImageTiling::eOptimal;
    vk::SampleCountFlagBits MsaaSampleCount = vk::SampleCountFlagBits::e1;
    vk::ImageView View;
    vk::Format Format = vk::Format::eUndefined;
    uint32_t Width = 0;
    uint32_t Height = 0;
    uint32_t MipCount = 0;
    uint32_t ArrayLayerCount = 0;
    size_t Size = 0;
};

class GPUImage
{
    public:

    GPUImage(uint32_t width, uint32_t height, uint32_t arrayLayers, vk::ImageCreateFlags createFlags, vk::ImageUsageFlags usageFlags, vk::Format format, uint32_t mipCount, vk::ImageTiling tiling, vk::SampleCountFlagBits msaaSampleCount);
    GPUImage(uint32_t width, uint32_t height, vk::ImageUsageFlags usageFlags, vk::Format format, uint32_t mipCount, vk::ImageTiling tiling = vk::ImageTiling::eOptimal, vk::SampleCountFlagBits msaaSampleCount = vk::SampleCountFlagBits::e1);
    GPUImage(uint32_t width, uint32_t height, vk::ImageUsageFlags usageFlags, vk::Format format, vk::ImageTiling tiling = vk::ImageTiling::eOptimal, vk::SampleCountFlagBits msaaSampleCount = vk::SampleCountFlagBits::e1);
    virtual ~GPUImage();

    GPUImage static CubeMap(uint32_t width, uint32_t height, vk::ImageUsageFlags usageFlags, vk::Format format, vk::ImageTiling tiling = vk::ImageTiling::eOptimal, vk::SampleCountFlagBits msaaSampleCount = vk::SampleCountFlagBits::e1);

    GPUImage(GPUImage&& other) noexcept;
    GPUImage& operator=(GPUImage&& other) noexcept;

    GPUImage(const GPUImage&) = delete;
    GPUImage& operator=(const GPUImage&) = delete;

    void UploadAndGenerateMip(void* data, uint32_t width, uint32_t height, uint32_t pixelStride, vk::CommandPool pool, vk::Queue queue);
    void TransitionLayout(vk::CommandPool pool, vk::Queue queue, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) const;

    inline ImageReference Internal() const { return _internal; }
    inline vk::ImageView View() const { return _internal.View; }
    inline uint32_t Width() const { return _internal.Width; }
    inline uint32_t Height() const { return _internal.Height; }
    inline uint32_t MipCount() const { return _internal.MipCount; }
    bool Resize(uint32_t width, uint32_t height);

    protected:
    void CreateImageInternal(uint32_t width, uint32_t height, uint32_t arrayLayers, vk::ImageCreateFlags createFlags, vk::ImageUsageFlags usageFlags, vk::Format format, uint32_t mipCount, vk::ImageTiling tiling, vk::SampleCountFlagBits msaaSampleCount);
    void DisposeInternal();
    DeviceMemory _memoryInternal = {};
    ImageReference _internal;
};
