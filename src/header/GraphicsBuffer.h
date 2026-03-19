#pragma once

#include "header/VkHandlers/VkPhysicalDeviceHandler.h"
#include <vulkan/vulkan_core.h>

class StageBuffer
{
    friend class GraphicsBuffer;
    public:
    StageBuffer(const VkPhysicalDeviceHandler& physicalDeviceHandler, VkDevice device, VkDeviceSize size);
    ~StageBuffer();

    StageBuffer(StageBuffer&& other) noexcept;
    StageBuffer& operator=(StageBuffer&& other) noexcept;

    StageBuffer(const StageBuffer&) = delete;
    StageBuffer& operator=(const StageBuffer&) = delete;

    void CopyToBuffer(const VkDevice& device, void* srcData, VkDeviceSize copySize);

    private:
    VkBuffer _internal;
    VkDeviceMemory _memoryInternal;
    VkDevice _device;
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
    GraphicsBuffer(const VkPhysicalDeviceHandler& physicalDeviceHandler, VkDevice device, VkDeviceSize count, VkDeviceSize stride, BufferType type);
    ~GraphicsBuffer();

    GraphicsBuffer(GraphicsBuffer&& other) noexcept;
    GraphicsBuffer& operator=(GraphicsBuffer&& other) noexcept;

    GraphicsBuffer(const GraphicsBuffer&) = delete;
    GraphicsBuffer& operator=(const GraphicsBuffer&) = delete;

    void CopyToBuffer(const VkPhysicalDeviceHandler& physicalDevice, const VkDevice& device, const VkQueue& queue, const VkCommandPool& cmdPool, void* srcData, VkDeviceSize size);

    VkBuffer GetInternal() const { return _internal; }
    BufferType GetType() const { return _type; }

    VkDeviceSize GetSize() const;
    VkDeviceSize GetCount() const;
    VkDeviceSize GetStride() const;

    void* GetDataPtr();

    private:
    VkBuffer _internal;
    VkDeviceMemory _memoryInternal;
    BufferType _type;
    void* _dataPtr; // Currently used for persistent mapping for Uniform buffers, we only map it once to avoid the cost of mapping it each time
    VkDeviceSize _stride;
    VkDeviceSize _count;
    VkDevice _device;
};
