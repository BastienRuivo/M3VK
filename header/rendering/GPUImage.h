#pragma once

#include <cstddef>
#include <cstdint>
#include <stb_image.h>
#include "handler/VkImageViewHandler.h"
#include <vulkan/vulkan_core.h>

struct ImageReference
{
    VkImage Image = VK_NULL_HANDLE;
    VkImageView View = VK_NULL_HANDLE;
    VkFormat Format = VK_FORMAT_UNDEFINED;
    uint32_t Width = 0;
    uint32_t Height = 0;
    uint32_t MipCount = 0;
    size_t Size = 0;
};

class GPUImage
{
    public:

    GPUImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryFlags);
    GPUImage(uint32_t width, uint32_t height, uint32_t mipCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryFlags);
    GPUImage(uint32_t width, uint32_t height, VkSampleCountFlagBits msaaSampleCount, uint32_t mipCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryFlags);
    ~GPUImage();

    GPUImage(GPUImage&& other) noexcept;
    GPUImage& operator=(GPUImage&& other) noexcept;

    GPUImage(const GPUImage&) = delete;
    GPUImage& operator=(const GPUImage&) = delete;

    void UploadAndGenerateMip(void* data, uint32_t width, uint32_t height, uint32_t pixelStride, VkCommandPool pool, VkQueue queue);
    void TransitionLayout(VkCommandPool pool, VkQueue queue, VkImageLayout oldLayout, VkImageLayout newLayout) const;

    inline ImageReference Internal() const { return _internal; }
    inline VkImageView View() const { return _internal.View; }
    inline uint32_t Width() const { return _internal.Width; }
    inline uint32_t Height() const { return _internal.Height; }
    inline uint32_t MipCount() const { return _internal.MipCount; }

    private:
    VkDeviceMemory _memoryInternal = VK_NULL_HANDLE;
    ImageReference _internal;
    VkImageViewHandler _view;
};
