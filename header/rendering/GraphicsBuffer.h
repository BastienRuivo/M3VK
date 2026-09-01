#pragma once

#include "allocation/MultiFrameRessource.h"
#include "application/DebugLayer.h"
#include "allocation/RessourceUsage.h"
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

class BindingManager;

class StageBuffer
{
    friend class GraphicsBuffer;
    public:

    enum Usage
    {
        Upload,
        Readback
    };

    StageBuffer(vk::DeviceSize size, Usage usage);
    virtual~StageBuffer();

    StageBuffer(StageBuffer&& other) noexcept;
    StageBuffer& operator=(StageBuffer&& other) noexcept;

    StageBuffer(const StageBuffer&) = delete;
    StageBuffer& operator=(const StageBuffer&) = delete;

    void* Map(vk::DeviceSize offset, vk::DeviceSize size);
    void Unmap();

    void MapAndCopyToBuffer(const void* srcData, vk::DeviceSize offset, vk::DeviceSize copySize);
    void MapAndCopyToData(void* dstData, vk::DeviceSize offset, vk::DeviceSize copySize);

    inline vk::Buffer Internal() const { return _internal; };
    inline DeviceMemory MemoryInternal() const { return _memoryInternal; };
    inline Usage Usage() const { return _usage; };
    inline vk::DeviceSize Capacity() const { return _capacity; };

    protected:
    vk::Buffer _internal = VK_NULL_HANDLE;
    DeviceMemory _memoryInternal = {};
    void* _data = nullptr;

    enum Usage _usage;
    vk::DeviceSize _capacity = 0;
};

class PoolStageBuffer : protected StageBuffer
{
    public:
    PoolStageBuffer(vk::DeviceSize size, enum Usage usage) : StageBuffer(size, usage), _offset(0) {}
    ~PoolStageBuffer() {}

    inline vk::DeviceSize Offset() const { return _offset; }

    void Map();
    void Unmap();
    void CopyToBuffer(const void* srcData, vk::DeviceSize copySize);
    void Clear();

    inline bool CanAllocate(vk::DeviceSize size) const { return _offset + size <= _capacity; }
    inline vk::Buffer Internal() const { return StageBuffer::Internal(); }

    private:
    uint32_t _offset = 0;
};

struct BufferInternal
{
    vk::Buffer Internal = VK_NULL_HANDLE;
    DeviceMemory MemoryInternal = {};
    void* DataPtr = nullptr; // Currently used for persistent mapping for Uniform buffers, we only map it once to avoid the cost of mapping it each time
    uint32_t GpuIndex = UINT32_MAX; // only filled if uniform or storage
};

class GraphicsBuffer : public MultiFrameRessource<BufferInternal>
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

    GraphicsBuffer(BindingManager& allocator, uint32_t dstBinding, BufferType type, RessourceUsage usage, vk::DeviceSize count, vk::DeviceSize stride, bool isSource = false);
    GraphicsBuffer(BufferType type, RessourceUsage usage, vk::DeviceSize count, vk::DeviceSize stride, bool isSource = false);

    ~GraphicsBuffer();

    GraphicsBuffer(GraphicsBuffer&& other) noexcept = default;
    GraphicsBuffer& operator=(GraphicsBuffer&& other) noexcept = default;

    GraphicsBuffer(const GraphicsBuffer&) = delete;
    GraphicsBuffer& operator=(const GraphicsBuffer&) = delete;

    void CopyToBuffer(const vk::Queue& queue,
        const vk::CommandPool& pool,
        void* srcData,
        vk::DeviceSize size,
        uint32_t srcIndex = 0,
        uint32_t dstIndex = 0);

    vk::Buffer Internal() const { return Current().Internal; }
    BufferType GetType() const { return _type; }

    inline RessourceUsage GetUsage() const { return _usage; }
    inline vk::DeviceSize GetSize() const { return _count * _stride; }
    inline vk::DeviceSize GetCount() const { return _count; }
    inline vk::DeviceSize GetStride() const { return _stride; }
    inline uint32_t GetGPUIndex() const { return Current().GpuIndex; }
    inline void* GetDataPtr() const
    {
        if(_usage != RessourceUsage::PerFrame)
        {
            DebugLayer::Log(DebugLayer::LogType::ERROR, "Trying to get data pointer for non uniform buffer");
        }
        return Current().DataPtr;
    }

    inline vk::DescriptorBufferInfo GetDescriptorBufferInfo(uint32_t index, uint32_t count) const
    {
        return vk::DescriptorBufferInfo{}
            .setBuffer(Current().Internal)
            .setOffset(index * _stride)
            .setRange(_stride * count);
    }

    inline vk::DescriptorType GetDescriptorType() const
    {
        switch (_type)
        {
        case BufferType::UNIFORM:
            return vk::DescriptorType::eUniformBuffer;
        case BufferType::STORAGE:
            return vk::DescriptorType::eStorageBuffer;
        case BufferType::INDIRECT_DRAW:
            return vk::DescriptorType::eStorageBuffer;
        default: throw std::runtime_error("Unimplemented Descriptor Type");
        }
    }

    protected:
    BufferInternal CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);

    BufferType _type;
    vk::DeviceSize _stride = 0;
    vk::DeviceSize _count = 0;
};

// this class handle a graphics buffer with an arbitrary huge size where we can append data
class GeometryBuffer : public GraphicsBuffer
{
    public:
    using GraphicsBuffer::GraphicsBuffer;

    GeometryBuffer(const GeometryBuffer&) = delete;
    GeometryBuffer& operator=(const GeometryBuffer&) = delete;

    GeometryBuffer(GeometryBuffer&& other) noexcept = default;
    GeometryBuffer& operator=(GeometryBuffer&& other) noexcept = default;

    void CopyToBuffer(const vk::Queue& queue,
        const vk::CommandPool& cmdPool,
        void* srcData,
        vk::DeviceSize size
    );

    inline vk::DeviceSize GetCurrentSize() const { return _currentSize; }
    inline uint32_t GetCurrentIndex() const { return _currentSize / _stride; }

    private:
    vk::DeviceSize _currentSize = 0;
};
