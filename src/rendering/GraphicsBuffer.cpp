#include "rendering/GraphicsBuffer.h"
#include "application/ApplicationInfo.h"
#include "rendering/CommandBuffer.h"
#include "rendering/DescriptorAllocator.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <vulkan/vulkan_core.h>

StageBuffer::StageBuffer(VkDeviceSize size, enum Usage bufferUsage)
: _capacity(size)
{
    VkBufferUsageFlags usage = bufferUsage == Upload ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VkBufferCreateInfo info
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = _capacity,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    // If note exclusive, need to add a queue family index

    if(vkCreateBuffer(ApplicationInfo::Device(), &info, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create buffer !");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(ApplicationInfo::Device(), _internal, &memRequirements);

    VkMemoryAllocateInfo allocateInfo
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        // mean it's a visible and writable by CPU directecly
        .memoryTypeIndex = ApplicationInfo::FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    if(vkAllocateMemory(ApplicationInfo::Device(), &allocateInfo, nullptr, &_memoryInternal) != VK_SUCCESS)
    {
        vkDestroyBuffer(ApplicationInfo::Device(), _internal, nullptr);
        throw std::runtime_error("Failed to allocate Stage Buffer memory");
    }

    if(vkBindBufferMemory(ApplicationInfo::Device(), _internal, _memoryInternal, 0) != VK_SUCCESS)
    {
        vkDestroyBuffer(ApplicationInfo::Device(), _internal, nullptr);
        vkFreeMemory(ApplicationInfo::Device(), _memoryInternal, nullptr);
        throw std::runtime_error("Failed to bind stage buffer memory");
    }
}

void StageBuffer::MapAndCopyToBuffer(void* srcData, VkDeviceSize offset, VkDeviceSize copySize)
{
    void* data;
    if(vkMapMemory(ApplicationInfo::Device(), _memoryInternal, offset, copySize, 0, &data) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to map stage buffer memory");
    }
    memcpy(data, srcData, (size_t)copySize);
    vkUnmapMemory(ApplicationInfo::Device(), _memoryInternal);
};

void StageBuffer::MapAndCopyToData(void* dstData, VkDeviceSize offset, VkDeviceSize copySize)
{
    void* data;
    if(vkMapMemory(ApplicationInfo::Device(), _memoryInternal, offset, copySize, 0, &data) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to map stage buffer memory");
    }
    memcpy(dstData, data, (size_t)copySize);
    vkUnmapMemory(ApplicationInfo::Device(), _memoryInternal);
}

void* StageBuffer::Map(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE)
{
    if(vkMapMemory(ApplicationInfo::Device(), _memoryInternal, offset, size, 0, &_data) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to map stage buffer memory");
    }

    return _data;
}

void StageBuffer::Unmap()
{
    vkUnmapMemory(ApplicationInfo::Device(), _memoryInternal);
    _data = nullptr;
}

StageBuffer::~StageBuffer()
{
    if(_data != nullptr)
    {
        vkUnmapMemory(ApplicationInfo::Device(), _memoryInternal);
    }
    vkDestroyBuffer(ApplicationInfo::Device(), _internal, nullptr);
    vkFreeMemory(ApplicationInfo::Device(), _memoryInternal, nullptr);
}

StageBuffer::StageBuffer(StageBuffer && other) noexcept
{
    _internal = other._internal;
    _memoryInternal = other._memoryInternal;

    other._internal = VK_NULL_HANDLE;
    other._memoryInternal = VK_NULL_HANDLE;
}

StageBuffer& StageBuffer::operator=(StageBuffer&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        _memoryInternal = other._memoryInternal;

        other._internal = VK_NULL_HANDLE;
        other._memoryInternal = VK_NULL_HANDLE;
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

void PoolStageBuffer::CopyToBuffer(void* srcData, VkDeviceSize copySize)
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

GraphicsBuffer::BufferInternal GraphicsBuffer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    BufferInternal buffer;

    VkBufferCreateInfo info
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    // // If note exclusive, need to add a queue family index

    if(vkCreateBuffer(ApplicationInfo::Device(), &info, nullptr, &buffer._internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create buffer !");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(ApplicationInfo::Device(), buffer._internal, &memRequirements);

    VkMemoryAllocateInfo allocateInfo
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = ApplicationInfo::FindMemoryType(memRequirements.memoryTypeBits, properties)
    };

    if(vkAllocateMemory(ApplicationInfo::Device(),&allocateInfo, nullptr, &buffer._memoryInternal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate buffer memory");
    }

    vkBindBufferMemory(ApplicationInfo::Device(), buffer._internal, buffer._memoryInternal, 0);

    if(_usage == RessourceUsage::PerFrame)
    {
        if(vkMapMemory(ApplicationInfo::Device(), buffer._memoryInternal, 0, memRequirements.size, 0, &buffer._dataPtr) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to map buffer memory");
        }
    }

    return buffer;
}

GraphicsBuffer::GraphicsBuffer(BufferType type, RessourceUsage bufferUsage, VkDeviceSize count, VkDeviceSize stride) : _type(type), _stride(stride), _count(count), _usage(bufferUsage)
{
    // mean it's a dst buffer, already in good memory shape but cant be writable directly by cpu
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    if(_type == UNIFORM || _type == STORAGE || _type == INDIRECT_DRAW)
    {
         VkDeviceSize alignement = stride;

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

    VkDeviceSize size = _stride * _count;

    switch (_type) {
        case INDEX: usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT; break;
        case VERTEX: usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; break;
        case UNIFORM: usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; break;
        case INDIRECT_DRAW: usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; break;
        case STORAGE: usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; break;
        default:
        {
            throw std::runtime_error("Achievement get :: How did we get Here ? (Uknown Buffer Type)");
        }
    }

    VkMemoryPropertyFlags properties;
    switch (_type) {
        case INDIRECT_DRAW:
        case UNIFORM:
        case STORAGE:
        case INDEX:
        case VERTEX: properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; break; // Memory optimized for GPU access
        default:
        {
            throw std::runtime_error("Achievement get :: How did we get Here ? (Uknown Buffer Type)");
        }
    }

    if(_usage == RessourceUsage::PerFrame)
    {
        if(_type != STORAGE && _type != UNIFORM && _type != INDIRECT_DRAW) throw std::runtime_error("PerFrame Buffer can only be STORAGE, UNIFORM or INDIRECT_DRAW");
        properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    int frameCount = ApplicationInfo::Constant::MaxFrameInFlight;
    if(_usage == RessourceUsage::Static)
    {
        frameCount = 1;
    }

    _buffers.resize(frameCount);
    for(int i = 0; i < _buffers.size(); i++)
    {
        _buffers[i] = CreateBuffer(size, usage, properties);
    }
}

GraphicsBuffer::GraphicsBuffer(DescriptorAllocator& allocator, uint32_t dstBinding, BufferType type, RessourceUsage bufferUsage, VkDeviceSize count, VkDeviceSize stride)
: GraphicsBuffer(type, bufferUsage, count, stride)
{
    for(int i = 0; i < _buffers.size(); i++)
    {
        if(_type == UNIFORM || _type == STORAGE || _type == INDIRECT_DRAW)
        {
            VkDescriptorBufferInfo desc = {
                .buffer = _buffers[i]._internal,
                .offset = 0,
                .range = _stride * _count
            };
            allocator.RegisterBuffer(desc, GetDescriptorType(), dstBinding, i);
            _buffers[i]._gpuIndex = i;
        }
    }
}

void GraphicsBuffer::CopyToBuffer(const VkQueue& queue,
    const VkCommandPool& pool,
    void* srcData,
    VkDeviceSize size,
    uint32_t srcIndex,
    uint32_t dstIndex)
{
    StageBuffer copyBuffer(size, StageBuffer::Usage::Upload);
    copyBuffer.MapAndCopyToBuffer(srcData, 0, size);

    CommandBuffer cmdBuffer(pool, queue);
    cmdBuffer.BeginSingleTime();
    {
        cmdBuffer.CopyBuffer(copyBuffer.Internal(), Current()._internal, size, srcIndex, dstIndex);
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
}

GraphicsBuffer::~GraphicsBuffer()
{
    for(int i = 0; i < _buffers.size(); i++)
    {
        vkDestroyBuffer(ApplicationInfo::Device(), _buffers[i]._internal, nullptr);
        if(_buffers[i]._dataPtr != nullptr) vkUnmapMemory(ApplicationInfo::Device(), _buffers[i]._memoryInternal);
        vkFreeMemory(ApplicationInfo::Device(), _buffers[i]._memoryInternal, nullptr);
    }
}

GraphicsBuffer::GraphicsBuffer(GraphicsBuffer && other) noexcept
{
    _buffers = std::move(other._buffers);

    _count = std::exchange(other._count, 0);
    _stride = std::exchange(other._stride, 0);
    _type = std::exchange(other._type, BufferType::VERTEX);
    _usage = std::exchange(other._usage, RessourceUsage::Static);
}

GraphicsBuffer& GraphicsBuffer::operator=(GraphicsBuffer&& other) noexcept
{
    if(this != &other)
    {
        _buffers = std::move(other._buffers);

        _count = std::exchange(other._count, 0);
        _stride = std::exchange(other._stride, 0);
        _type = std::exchange(other._type, BufferType::VERTEX);
        _usage = std::exchange(other._usage, RessourceUsage::Static);
    }

    return *this;
}

void GeometryBuffer::CopyToBuffer(const VkQueue& queue,
    const VkCommandPool& cmdPool,
    void* srcData,
    VkDeviceSize size)
{
    if((_currentSize + size) > (_count * _stride))
    {
        DebugLayer::Log(DebugLayer::LogType::ERROR, "Max vertex buffer size reached");
        throw std::runtime_error("Max vertex buffer size reached");
    }

    uint32_t index = 0;

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
