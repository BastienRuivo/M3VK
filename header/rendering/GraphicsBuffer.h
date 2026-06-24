#pragma once

#include "application/ApplicationInfo.h"
#include "application/DebugLayer.h"
#include "rendering/DescriptorAllocator.h"
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

class StageBuffer
{
    friend class GraphicsBuffer;
    public:

    enum Usage
    {
        Upload,
        Readback
    };

    StageBuffer(VkDeviceSize size, Usage usage);
    virtual~StageBuffer();

    StageBuffer(StageBuffer&& other) noexcept;
    StageBuffer& operator=(StageBuffer&& other) noexcept;

    StageBuffer(const StageBuffer&) = delete;
    StageBuffer& operator=(const StageBuffer&) = delete;

    void* Map(VkDeviceSize offset, VkDeviceSize size);
    void Unmap();

    void MapAndCopyToBuffer(const void* srcData, VkDeviceSize offset, VkDeviceSize copySize);
    void MapAndCopyToData(void* dstData, VkDeviceSize offset, VkDeviceSize copySize);

    inline VkBuffer Internal() const { return _internal; };
    inline VkDeviceMemory MemoryInternal() const { return _memoryInternal; };
    inline Usage Usage() const { return _usage; };
    inline VkDeviceSize Capacity() const { return _capacity; };

    protected:
    VkBuffer _internal = VK_NULL_HANDLE;
    VkDeviceMemory _memoryInternal = VK_NULL_HANDLE;
    void* _data = nullptr;

    enum Usage _usage;
    VkDeviceSize _capacity = 0;
};

class PoolStageBuffer : protected StageBuffer
{
    public:
    PoolStageBuffer(VkDeviceSize size, enum Usage usage) : StageBuffer(size, usage), _offset(0) {}
    ~PoolStageBuffer() {}

    inline VkDeviceSize Offset() const { return _offset; }

    void Map();
    void Unmap();
    void CopyToBuffer(const void* srcData, VkDeviceSize copySize);
    void Clear();

    inline bool CanAllocate(VkDeviceSize size) const { return _offset + size <= _capacity; }
    inline VkBuffer Internal() const { return StageBuffer::Internal(); }

    private:
    uint32_t _offset = 0;
};

class GraphicsBuffer
{
    public:
    enum BufferType
    {
        INDEX,
        VERTEX,
        UNIFORM,
        STORAGE,
        INDIRECT_DRAW,
        COUNT
    };

    GraphicsBuffer(DescriptorAllocator& allocator, uint32_t dstBinding, BufferType type, RessourceUsage usage, VkDeviceSize count, VkDeviceSize stride, bool isSource = false);
    GraphicsBuffer(BufferType type, RessourceUsage usage, VkDeviceSize count, VkDeviceSize stride, bool isSource = false);

    ~GraphicsBuffer();

    GraphicsBuffer(GraphicsBuffer&& other) noexcept;
    GraphicsBuffer& operator=(GraphicsBuffer&& other) noexcept;

    GraphicsBuffer(const GraphicsBuffer&) = delete;
    GraphicsBuffer& operator=(const GraphicsBuffer&) = delete;

    void CopyToBuffer(const VkQueue& queue,
        const VkCommandPool& pool,
        void* srcData,
        VkDeviceSize size,
        uint32_t srcIndex = 0,
        uint32_t dstIndex = 0);

    VkBuffer Internal() const { return Current()._internal; }
    BufferType GetType() const { return _type; }

    inline RessourceUsage GetUsage() const { return _usage; }
    inline VkDeviceSize GetSize() const { return _count * _stride; }
    inline VkDeviceSize GetCount() const { return _count; }
    inline VkDeviceSize GetStride() const { return _stride; }
    inline uint32_t GetGPUIndex() const { return Current()._gpuIndex; }
    inline void* GetDataPtr() const
    {
        if(_usage != RessourceUsage::PerFrame)
        {
            DebugLayer::Log(DebugLayer::LogType::ERROR, "Trying to get data pointer for non uniform buffer");
        }
        return Current()._dataPtr;
    }

    inline VkDescriptorBufferInfo GetDescriptorBufferInfo(uint32_t index, uint32_t count) const
    {
        return
        {
            .buffer = Current()._internal,
            .offset = index * _stride,
            .range = _stride * count
        };
    }

    inline VkDescriptorType GetDescriptorType() const
    {
        switch (_type)
        {
        case BufferType::UNIFORM:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case BufferType::STORAGE:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case BufferType::INDIRECT_DRAW:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        default: throw std::runtime_error("Unimplemented Descriptor Type");
        }
    }

    protected:

    struct BufferInternal
    {
        VkBuffer _internal = VK_NULL_HANDLE;
        VkDeviceMemory _memoryInternal = VK_NULL_HANDLE;
        void* _dataPtr = nullptr; // Currently used for persistent mapping for Uniform buffers, we only map it once to avoid the cost of mapping it each time
        uint32_t _gpuIndex = UINT32_MAX; // only filled if uniform or storage
    };

    inline const BufferInternal& Current() const { return _usage == RessourceUsage::Static ? _buffers[0] : _buffers[ApplicationInfo::CurrentFrame()]; }
    BufferInternal CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

    std::vector<BufferInternal> _buffers;
    BufferType _type;
    RessourceUsage _usage;
    VkDeviceSize _stride = 0;
    VkDeviceSize _count = 0;
};

// this class handle a graphics buffer with an arbitrary huge size where we can append data
class GeometryBuffer : public GraphicsBuffer
{
    public:
    using GraphicsBuffer::GraphicsBuffer;

    GeometryBuffer(const GeometryBuffer&) = delete;
    GeometryBuffer& operator=(const GeometryBuffer&) = delete;

    GeometryBuffer(GeometryBuffer&& other) noexcept;
    GeometryBuffer& operator=(GeometryBuffer&& other) noexcept;

    void CopyToBuffer(const VkQueue& queue,
        const VkCommandPool& cmdPool,
        void* srcData,
        VkDeviceSize size
    );

    inline VkDeviceSize GetCurrentSize() const { return _currentSize; }
    inline uint32_t GetCurrentIndex() const { return _currentSize / _stride; }

    private:
    VkDeviceSize _currentSize = 0;
};
