#include "rendering/GraphicsBuffer.h"
#include "application/ApplicationInfo.h"
#include "rendering/CommandBuffer.h"
#include "allocation/BindingManager.h"
#include "allocation/RessourceUsage.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <vulkan/vulkan.hpp>

StageBuffer::StageBuffer(vk::DeviceSize size, enum Usage bufferUsage)
: _capacity(size)
{
    vk::BufferUsageFlags usage = bufferUsage == Upload ? vk::BufferUsageFlagBits::eTransferSrc : vk::BufferUsageFlagBits::eTransferDst;

    vk::BufferCreateInfo info = vk::BufferCreateInfo{}
        .setSize(_capacity)
        .setUsage(usage)
        .setSharingMode(vk::SharingMode::eExclusive);

    // If note exclusive, need to add a queue family index

    if(ApplicationInfo::Device().createBuffer(&info, nullptr, &_internal) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create buffer !");
    }

    vk::MemoryRequirements memRequirements = ApplicationInfo::Device().getBufferMemoryRequirements(_internal);

    // mean it's a visible and writable by CPU directecly
    vk::MemoryAllocateInfo allocateInfo = vk::MemoryAllocateInfo{}
        .setAllocationSize(memRequirements.size)
        .setMemoryTypeIndex(ApplicationInfo::FindMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));

    vk::Result memoryResult = ApplicationInfo::Device().allocateMemory(&allocateInfo, nullptr, &_memoryInternal.Memory);
    if(memoryResult != vk::Result::eSuccess)
    {
        ApplicationInfo::Device().destroyBuffer(_internal);
        throw std::runtime_error("Failed to allocate Stage Buffer memory");
    }
    _memoryInternal.Size = allocateInfo.allocationSize;
    ApplicationInfo::VRAMAllocate(_memoryInternal.Size, ApplicationInfo::AllocType::Buffer);

    try
    {
        ApplicationInfo::Device().bindBufferMemory(_internal, _memoryInternal.Memory, 0);
    }
    catch(const std::exception&)
    {
        ApplicationInfo::Device().destroyBuffer(_internal);
        ApplicationInfo::Device().freeMemory(_memoryInternal.Memory);
        ApplicationInfo::VRAMRelease(_memoryInternal.Size, ApplicationInfo::AllocType::Buffer);
        throw std::runtime_error("Failed to bind stage buffer memory");
    }
}

void StageBuffer::MapAndCopyToBuffer(const void* srcData, vk::DeviceSize offset, vk::DeviceSize copySize)
{
    void* data;
    if(ApplicationInfo::Device().mapMemory(_memoryInternal.Memory, offset, copySize, {}, &data) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to map stage buffer memory");
    }
    memcpy(data, srcData, (size_t)copySize);
    ApplicationInfo::Device().unmapMemory(_memoryInternal.Memory);
};

void StageBuffer::MapAndCopyToData(void* dstData, vk::DeviceSize offset, vk::DeviceSize copySize)
{
    void* data;
    if(ApplicationInfo::Device().mapMemory(_memoryInternal.Memory, offset, copySize, {}, &data) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to map stage buffer memory");
    }
    memcpy(dstData, data, (size_t)copySize);
    ApplicationInfo::Device().unmapMemory(_memoryInternal.Memory);
}

void* StageBuffer::Map(vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE)
{
    if(ApplicationInfo::Device().mapMemory(_memoryInternal.Memory, offset, size, {}, &_data) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to map stage buffer memory");
    }

    return _data;
}

void StageBuffer::Unmap()
{
    ApplicationInfo::Device().unmapMemory(_memoryInternal.Memory);
    _data = nullptr;
}

StageBuffer::~StageBuffer()
{
    if(_data != nullptr)
    {
        ApplicationInfo::Device().unmapMemory(_memoryInternal.Memory);
    }
    ApplicationInfo::Device().destroyBuffer(_internal);
    ApplicationInfo::Device().freeMemory(_memoryInternal.Memory);
    ApplicationInfo::VRAMRelease(_memoryInternal.Size, ApplicationInfo::AllocType::Buffer);
}

StageBuffer::StageBuffer(StageBuffer && other) noexcept
{
    _internal = other._internal;
    _memoryInternal = other._memoryInternal;

    other._internal = VK_NULL_HANDLE;
    other._memoryInternal = {};
}

StageBuffer& StageBuffer::operator=(StageBuffer&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        _memoryInternal = other._memoryInternal;

        other._internal = VK_NULL_HANDLE;
        other._memoryInternal = {};
    }

    return *this;
}

void PoolStageBuffer::Map()
{
    StageBuffer::Map();
}

void PoolStageBuffer::Unmap()
{
    StageBuffer::Unmap();
}

void PoolStageBuffer::CopyToBuffer(const void* srcData, vk::DeviceSize copySize)
{
    if(_data == nullptr)
    {
        throw std::runtime_error("Buffer is not mapped !");
    }

    if(!CanAllocate(copySize))
    {
        throw std::runtime_error("Buffer is full !");
    }

    std::byte* data = static_cast<std::byte*>(_data) + _offset;
    memcpy(data, srcData, (size_t)copySize);
    _offset += copySize;
    uint32_t stride = ApplicationInfo::GetProperties().limits.minUniformBufferOffsetAlignment;
    _offset = std::ceil(_offset / (float)stride) * stride;
}

void PoolStageBuffer::Clear()
{
    _offset = 0;
}

BufferInternal GraphicsBuffer::CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
    BufferInternal buffer;

    vk::BufferCreateInfo info = vk::BufferCreateInfo{}
        .setSize(size)
        .setUsage(usage)
        .setSharingMode(vk::SharingMode::eExclusive);

    // // If note exclusive, need to add a queue family index

    if(ApplicationInfo::Device().createBuffer(&info, nullptr, &buffer.Internal) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create buffer !");
    }

    vk::MemoryRequirements memRequirements = ApplicationInfo::Device().getBufferMemoryRequirements(buffer.Internal);

    vk::MemoryAllocateInfo allocateInfo = vk::MemoryAllocateInfo{}
        .setAllocationSize(memRequirements.size)
        .setMemoryTypeIndex(ApplicationInfo::FindMemoryType(memRequirements.memoryTypeBits, properties));

    vk::Result memoryResult = ApplicationInfo::Device().allocateMemory(&allocateInfo, nullptr, &buffer.MemoryInternal.Memory);
    if(memoryResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate buffer memory");
    }
    buffer.MemoryInternal.Size = allocateInfo.allocationSize;
    ApplicationInfo::VRAMAllocate(allocateInfo.allocationSize, ApplicationInfo::AllocType::Buffer);

    try
    {
        ApplicationInfo::Device().bindBufferMemory(buffer.Internal, buffer.MemoryInternal.Memory, 0);
    }
    catch(const std::exception&)
    {
        throw std::runtime_error("Can't bind buffer memory");
    }

    if(_usage == RessourceUsage::PerFrame)
    {
        if(ApplicationInfo::Device().mapMemory(buffer.MemoryInternal.Memory, 0, memRequirements.size, {}, &buffer.DataPtr) != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to map buffer memory");
        }
    }

    return buffer;
}

GraphicsBuffer::GraphicsBuffer(BufferType type, RessourceUsage bufferUsage, vk::DeviceSize count, vk::DeviceSize stride, bool isSource) : _type(type), _stride(stride), _count(count)
{
    _usage = bufferUsage;
    // mean it's a dst buffer, already in good memory shape but cant be writable directly by cpu
    vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eTransferDst;

    if(isSource)
    {
        usage |= vk::BufferUsageFlagBits::eTransferSrc;
    }

    if(_type == UNIFORM || _type == STORAGE || _type == INDIRECT_DRAW)
    {
         vk::DeviceSize alignement = stride;

        switch(_type) {
            case UNIFORM: alignement = ApplicationInfo::GetProperties().limits.minUniformBufferOffsetAlignment; break;
            case INDIRECT_DRAW:
            case STORAGE: alignement = ApplicationInfo::GetProperties().limits.minStorageBufferOffsetAlignment; break;
            default:
            {
                throw std::runtime_error("Achievement get :: How did we get Here ? (Uknown Buffer Type)");
            }
        }

        if(stride % alignement != 0)
        {
            DebugLayer::Log(DebugLayer::LogType::WARNING, "Stride is not multiple of alignement ! Alignement = " + std::to_string(alignement) + " Stride = " + std::to_string(stride));
        }
        _stride = (stride + alignement - 1) & ~(alignement - 1);
    }

    vk::DeviceSize size = _stride * _count;

    switch (_type) {
        case INDEX: usage |= vk::BufferUsageFlagBits::eIndexBuffer; break;
        case VERTEX: usage |= vk::BufferUsageFlagBits::eVertexBuffer; break;
        case UNIFORM: usage |= vk::BufferUsageFlagBits::eUniformBuffer; break;
        case INDIRECT_DRAW: usage |= vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eStorageBuffer; break;
        case STORAGE: usage |= vk::BufferUsageFlagBits::eStorageBuffer; break;
        default:
        {
            throw std::runtime_error("Achievement get :: How did we get Here ? (Uknown Buffer Type)");
        }
    }

    vk::MemoryPropertyFlags properties;
    switch (_type) {
        case INDIRECT_DRAW:
        case UNIFORM:
        case STORAGE:
        case INDEX:
        case VERTEX: properties = vk::MemoryPropertyFlagBits::eDeviceLocal; break; // Memory optimized for GPU access
        default:
        {
            throw std::runtime_error("Achievement get :: How did we get Here ? (Uknown Buffer Type)");
        }
    }

    if(_usage == RessourceUsage::PerFrame)
    {
        if(_type != STORAGE && _type != UNIFORM && _type != INDIRECT_DRAW) throw std::runtime_error("PerFrame Buffer can only be STORAGE, UNIFORM or INDIRECT_DRAW");
        properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    }

    int frameCount = RessourceUsageCount(_usage);

    _internals.resize(frameCount);
    for(int i = 0; i < _internals.size(); i++)
    {
        _internals[i] = CreateBuffer(size, usage, properties);
    }
}

GraphicsBuffer::GraphicsBuffer(BindingManager& allocator, uint32_t dstBinding, BufferType type, RessourceUsage bufferUsage, vk::DeviceSize count, vk::DeviceSize stride, bool isSource)
: GraphicsBuffer(type, bufferUsage, count, stride, isSource)
{
    for(int i = 0; i < _internals.size(); i++)
    {
        if(_type == UNIFORM || _type == STORAGE || _type == INDIRECT_DRAW)
        {
            vk::DescriptorBufferInfo desc = vk::DescriptorBufferInfo{}
                .setBuffer(_internals[i].Internal)
                .setOffset(0)
                .setRange(_stride * _count);
            allocator.RegisterBuffer(desc, GetDescriptorType(), dstBinding, i);
            _internals[i].GpuIndex = i;
        }
    }
}

void GraphicsBuffer::CopyToBuffer(const vk::Queue& queue,
    const vk::CommandPool& pool,
    void* srcData,
    vk::DeviceSize size,
    uint32_t srcOffsetInBytes,
    uint32_t dstOffsetInBytes)
{
    StageBuffer copyBuffer(size, StageBuffer::Usage::Upload);
    copyBuffer.MapAndCopyToBuffer(srcData, 0, size);

    CommandBuffer cmdBuffer(pool, queue);
    cmdBuffer.BeginSingleTime();
    {
        cmdBuffer.CopyBuffer(copyBuffer.Internal(), Current().Internal, size, srcOffsetInBytes, dstOffsetInBytes);
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
}

GraphicsBuffer::~GraphicsBuffer()
{
    for(int i = 0; i < _internals.size(); i++)
    {
        ApplicationInfo::Device().destroyBuffer(_internals[i].Internal);
        if(_internals[i].DataPtr != nullptr) ApplicationInfo::Device().unmapMemory(_internals[i].MemoryInternal.Memory);
        ApplicationInfo::Device().freeMemory(_internals[i].MemoryInternal.Memory);
        ApplicationInfo::VRAMRelease(_internals[i].MemoryInternal.Size, ApplicationInfo::AllocType::Buffer);
    }
}

GraphicsBuffer::GraphicsBuffer(GraphicsBuffer && other) noexcept
{
    _internals = std::move(other._internals);

    _count = std::exchange(other._count, 0);
    _stride = std::exchange(other._stride, 0);
    _type = std::exchange(other._type, BufferType::VERTEX);
    _usage = std::exchange(other._usage, RessourceUsage::Static);
}

GraphicsBuffer& GraphicsBuffer::operator=(GraphicsBuffer&& other) noexcept
{
    if(this != &other)
    {
        _internals = std::move(other._internals);

        _count = std::exchange(other._count, 0);
        _stride = std::exchange(other._stride, 0);
        _type = std::exchange(other._type, BufferType::VERTEX);
        _usage = std::exchange(other._usage, RessourceUsage::Static);
    }

    return *this;
}

void GeometryBuffer::CopyToBuffer(const vk::Queue& queue,
    const vk::CommandPool& cmdPool,
    void* srcData,
    vk::DeviceSize size)
{
    if((_currentSize + size) > (_count * _stride))
    {
        DebugLayer::Log(DebugLayer::LogType::ERROR, "Max vertex buffer size reached");
        throw std::runtime_error("Max vertex buffer size reached");
    }

    GraphicsBuffer::CopyToBuffer(queue, cmdPool, srcData, size, 0, _currentSize);
    _currentSize += size;
}

GeometryBuffer::GeometryBuffer(GeometryBuffer&& other) noexcept
    : GraphicsBuffer(std::move(other)),
      _currentSize(std::exchange(other._currentSize, 0))
{

}

GeometryBuffer& GeometryBuffer::operator=(GeometryBuffer&& other) noexcept
{
    if(this != &other)
    {
        GraphicsBuffer::operator=(std::move(other));
        _currentSize = std::exchange(other._currentSize, 0);
    }
    return *this;
}
