#pragma once

#include <cstdint>
#include <stb_image.h>
#include "header/CPUImage.h"
#include "header/CommandBuffer.h"
#include "header/VkHandlers/VkImageViewHandler.h"
#include <vulkan/vulkan_core.h>

class GPUImage
{
    public:
    virtual ~GPUImage() = default;
    GPUImage(const GPUImage&)            = delete;
    GPUImage& operator=(const GPUImage&) = delete;

    // Getter
    inline VkImage Get() const { return _internal; }
    inline VkImageView GetView() const { return _view.Get(); }
    inline VkFormat GetFormat() const { return _format; }
    inline uint32_t GetMipCount() const { return _mipCount; }
    inline uint32_t GetWidth() const { return _width; }
    inline uint32_t GetHeight() const { return _height; }

    void TransitionLayoutCommand(const CommandBuffer& cmdBuffer, VkImageLayout oldLayout, VkImageLayout newLayout) const;

    protected:
    GPUImage() = default;

    VkImageViewHandler _view;
    VkImage _internal = VK_NULL_HANDLE;
    VkFormat _format = VK_FORMAT_UNDEFINED;
    uint32_t _width = 0;
    uint32_t _height = 0;
    uint32_t _mipCount = 0;
};

class GPUAllocatedImage : public GPUImage
{
    public:

    using GPUImage::GPUImage;

    GPUAllocatedImage(const CPUImage& cpuImg, VkCommandPool pool, VkQueue queue);
    GPUAllocatedImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryFlags);
    GPUAllocatedImage(uint32_t width, uint32_t height, uint32_t mipCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryFlags);
    GPUAllocatedImage(uint32_t width, uint32_t height, VkSampleCountFlagBits msaaSampleCount, uint32_t mipCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryFlags);
    ~GPUAllocatedImage();

    GPUAllocatedImage(GPUAllocatedImage&& other) noexcept;
    GPUAllocatedImage& operator=(GPUAllocatedImage&& other) noexcept;

    GPUAllocatedImage(const GPUAllocatedImage&) = delete;
    GPUAllocatedImage& operator=(const GPUAllocatedImage&) = delete;

    void CopyCPUtoGPUImage(const CPUImage & cpuImg, VkCommandPool pool, VkQueue queue);
    void TransitionLayout(VkCommandPool pool, VkQueue queue, VkImageLayout oldLayout, VkImageLayout newLayout) const;
    void GenerateMipmapsCommand(const CommandBuffer& cmdBuffer) const;

    private:
    VkDeviceMemory _memoryInternal = VK_NULL_HANDLE;
};
