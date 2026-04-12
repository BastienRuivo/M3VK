#pragma once

#include <cstdint>
#include <stb_image.h>
#include "header/CommandBuffer.h"
#include "header/VkHandlers/VkImageViewHandler.h"
#include "header/VkHandlers/VkPhysicalDeviceHandler.h"
#include <string>
#include <vulkan/vulkan_core.h>
class CPUImage
{
    public:
    CPUImage(const std::string& path, int channelFormat);
    ~CPUImage();

    CPUImage(CPUImage&& other) noexcept;
    CPUImage& operator=(CPUImage&& other) noexcept;

    CPUImage(const CPUImage&) = delete;
    CPUImage& operator=(const CPUImage&) = delete;

    VkFormat GetGPUFormat() const;

    inline VkDeviceSize Size() const { return _width * _height* _channels; }
    inline int Width() const { return _width; };
    inline int Height() const { return _height; };
    inline int Channels() const { return _channels; };
    inline stbi_uc* Data() const { return _data; };

    private:
    int _width = 0;
    int _height = 0;
    int _channels = 0;

    stbi_uc* _data = nullptr;
};

class GPUImage
{
    public:
    GPUImage(VkDevice device, const VkPhysicalDeviceHandler & physicalDevice,  const CPUImage& cpuImg, VkCommandPool pool, VkQueue queue);
    GPUImage(VkDevice device, const VkPhysicalDeviceHandler & physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryFlags);
    GPUImage(VkDevice device, const VkPhysicalDeviceHandler & physicalDevice, uint32_t width, uint32_t height, uint32_t mipCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryFlags);
    GPUImage(VkDevice device, const VkPhysicalDeviceHandler & physicalDevice, uint32_t width, uint32_t height, VkSampleCountFlagBits msaaSampleCount, uint32_t mipCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags imageUsageFlags, VkMemoryPropertyFlags memoryFlags);
    ~GPUImage();

    GPUImage(GPUImage&& other) noexcept;
    GPUImage& operator=(GPUImage&& other) noexcept;

    GPUImage(const GPUImage&) = delete;
    GPUImage& operator=(const GPUImage&) = delete;

    void CopyCPUtoGPUImage(const CPUImage & cpuImg, const VkPhysicalDeviceHandler& physicalDevice, VkCommandPool pool, VkQueue queue);
    void TransitionLayout(VkCommandPool pool, VkQueue queue, VkImageLayout oldLayout, VkImageLayout newLayout);
    void TransitionLayoutCommand(const CommandBuffer& cmdBuffer, VkImageLayout oldLayout, VkImageLayout newLayout);
    void GenerateMipmapsCommand(const CommandBuffer& cmdBuffer, const VkPhysicalDeviceHandler& physicalDevice);

    inline VkImage Get() const { return _internal; }
    inline VkImageView GetView() const { return _view.Get(); }
    inline VkFormat GetFormat() const { return _format; }
    inline uint32_t GetMipCount() const { return _mipCount; }
    inline uint32_t GetWidth() const { return _width; }
    inline uint32_t GetHeight() const { return _height; }

    private:
    uint32_t _mipCount;
    VkImage _internal = VK_NULL_HANDLE;
    VkDeviceMemory _memoryInternal = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    VkFormat _format;
    VkImageViewHandler _view;
    uint32_t _width, _height;
};
