#pragma once

#include "header/DebugLayer.h"
#include "header/VkHandlers/VkPhysicalDeviceHandler.h"
#include <cstdint>
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
    inline VkBuffer Get() const { return _internal; };

    private:
    VkBuffer _internal = VK_NULL_HANDLE;
    VkDeviceMemory _memoryInternal = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
};


class GraphicsBuffer
{
    public:
    enum BufferType
    {
        INDEX = 0,
        VERTEX = 1,
        UNIFORM = 2,
        STATIC_STORAGE = 3
    };
    GraphicsBuffer(const VkPhysicalDeviceHandler& physicalDeviceHandler, VkDevice device, VkDeviceSize count, VkDeviceSize stride, BufferType type);
    ~GraphicsBuffer();

    GraphicsBuffer(GraphicsBuffer&& other) noexcept;
    GraphicsBuffer& operator=(GraphicsBuffer&& other) noexcept;

    GraphicsBuffer(const GraphicsBuffer&) = delete;
    GraphicsBuffer& operator=(const GraphicsBuffer&) = delete;

    void CopyToBuffer(const VkPhysicalDeviceHandler& physicalDevice,
        const VkDevice& device,
        const VkQueue& queue,
        const VkCommandPool& pool,
        void* srcData,
        VkDeviceSize size,
        uint32_t srcIndex = 0,
        uint32_t dstIndex = 0);

    VkBuffer Get() const { return _internal; }
    BufferType GetType() const { return _type; }

    inline VkDeviceSize GetSize() const { return _count * _stride; }
    inline VkDeviceSize GetCount() const { return _count; }
    inline VkDeviceSize GetStride() const { return _stride; }
    inline void* GetDataPtr() const
    {
        if(_type != BufferType::UNIFORM)
        {
            DebugLayer::Log(DebugLayer::LogType::ERROR, "Trying to get data pointer for non uniform buffer");
        }
        return _dataPtr;
    }

    protected:
    VkBuffer _internal;
    VkDeviceMemory _memoryInternal;
    BufferType _type;
    void* _dataPtr; // Currently used for persistent mapping for Uniform buffers, we only map it once to avoid the cost of mapping it each time
    VkDeviceSize _stride;
    VkDeviceSize _count;
    VkDevice _device;
};

// this class handle a graphics buffer with an arbitrary huge size where we can append data
class GeometryBuffer : public GraphicsBuffer
{
    public:
    using GraphicsBuffer::GraphicsBuffer;

    void CopyToBuffer(const VkPhysicalDeviceHandler& physicalDevice,
        const VkDevice& device,
        const VkQueue& queue,
        const VkCommandPool& cmdPool,
        void* srcData,
        VkDeviceSize size
    );

    inline VkDeviceSize GetCurrentSize() const { return _currentSize; }
    inline uint32_t GetCurrentIndex() const { return _currentSize / _stride; }

    private:
    VkDeviceSize _currentSize = 0;
};
