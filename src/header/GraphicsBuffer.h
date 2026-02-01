#pragma once

#include <vulkan/vulkan_core.h>

class StageBuffer
{
    friend class GraphicsBuffer;
    public:
    void Create(const VkPhysicalDevice& physicalDevice, const VkDevice& device, VkDeviceSize size);
    void DisposeBuffer(const VkDevice& device);

    void CopyToBuffer(const VkDevice& device, void* srcData, VkDeviceSize copySize);

    private:
    VkBuffer _buffer;
    VkDeviceMemory _memory;
};


class GraphicsBuffer
{
    public:
    enum BufferType
    {
        INDEX = 0,
        VERTEX = 1,
        UNIFORM = 2
    };
    void Create(const VkPhysicalDevice& physicalDevice, const VkDevice& device, VkDeviceSize count, VkDeviceSize stride, BufferType type);
    void DisposeBuffer(const VkDevice& device);

    void CopyToBuffer(const VkPhysicalDevice& physicalDevice, const VkDevice& device, const VkQueue& queue, const VkCommandPool& cmdPool, void* srcData, VkDeviceSize size);

    VkBuffer GetInternal() const { return _buffer; }
    BufferType GetType() const { return _type; }

    VkDeviceSize GetSize() const;
    VkDeviceSize GetCount() const;
    VkDeviceSize GetStride() const;

    void* GetDataPtr();

    private:
    VkBuffer _buffer;
    VkDeviceMemory _memory;
    BufferType _type;
    void* _dataPtr; // Currently used for persistent mapping for Uniform buffers, we only map it once to avoid the cost of mapping it each time
    VkDeviceSize _stride;
    VkDeviceSize _count;
};
