#include "header/GraphicsBuffer.h"
#include "header/VkHandlers/VkPhysicalDeviceHandler.h"
#include <cstddef>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

StageBuffer::StageBuffer(const VkPhysicalDeviceHandler& physicalDeviceHandler, VkDevice device, VkDeviceSize size)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "StageBuffer creation !");
#endif

    _device = device;

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateBuffer(_device, &info, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create buffer !");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_device, _internal, &memRequirements);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memRequirements.size;
    // mean it's a visible and writable by CPU directecly
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    allocateInfo.memoryTypeIndex = physicalDeviceHandler.FindMemoryType(memRequirements.memoryTypeBits, properties);

    if(vkAllocateMemory(_device, &allocateInfo, nullptr, &_memoryInternal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Stage Buffer memory");
    }

    if(vkBindBufferMemory(_device, _internal, _memoryInternal, 0) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to bind stage buffer memory");
    }
}

void StageBuffer::CopyToBuffer(const VkDevice& device, void* srcData, VkDeviceSize copySize)
{
    void* data;
    vkMapMemory(device, _memoryInternal, 0, copySize, 0, &data);
    memcpy(data, srcData, (size_t)copySize);
    vkUnmapMemory(device, _memoryInternal);
};

StageBuffer::~StageBuffer()
{
    vkDestroyBuffer(_device, _internal, nullptr);
    vkFreeMemory(_device, _memoryInternal, nullptr);
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "StageBuffer Destroyed !");
#endif
}

StageBuffer::StageBuffer(StageBuffer && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "StageBuffer Move Creation !");
#endif

    _internal = other._internal;
    _memoryInternal = other._memoryInternal;
    _device = other._device;

    other._internal = VK_NULL_HANDLE;
    other._memoryInternal = VK_NULL_HANDLE;
}

StageBuffer& StageBuffer::operator=(StageBuffer&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        _memoryInternal = other._memoryInternal;
        _device = other._device;
        other._internal = VK_NULL_HANDLE;
        other._memoryInternal = VK_NULL_HANDLE;
    }

    return *this;
}

void* GraphicsBuffer::GetDataPtr()
{
    return _dataPtr;
}

VkDeviceSize GraphicsBuffer::GetSize() const
{
    return GetCount() * GetStride();
}

VkDeviceSize GraphicsBuffer::GetCount() const
{
    return _count;
}

VkDeviceSize GraphicsBuffer::GetStride() const
{
    return _stride;
}

GraphicsBuffer::GraphicsBuffer(const VkPhysicalDeviceHandler& physicalDevice, VkDevice device, VkDeviceSize count, VkDeviceSize stride, BufferType type)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "GraphicsBuffer Creation !");
#endif
    _device = device;
    // mean it's a dst buffer, already in good memory shape but cant be writable directly by cpu
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    // mean it's a GPU buffer
    VkMemoryPropertyFlags properties;

    _type = type;
    _dataPtr = nullptr;
    _stride = stride;
    _count = count;

    VkDeviceSize size = _stride * _count;

    // Usage
    switch (_type) {
        case INDEX: usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT; break;
        case VERTEX: usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; break;
        case UNIFORM: usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; break;
        case STATIC_STORAGE: usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; break;
        default:
        {
            throw std::runtime_error("Achievement get :: How did we get Here ? (Uknown Buffer Type)");
        }
    }

    // Memory type
    switch (_type) {
        case STATIC_STORAGE:
        case INDEX:
        case VERTEX: properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; break; // Memory optimized for GPU access
        case UNIFORM: properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; // Host = CPU so it mean it is visible and writable by it
    }

    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateBuffer(device, &info, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create buffer !");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, _internal, &memRequirements);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memRequirements.size;
    allocateInfo.memoryTypeIndex = physicalDevice.FindMemoryType(memRequirements.memoryTypeBits, properties);

    if(vkAllocateMemory(device, &allocateInfo, nullptr, &_memoryInternal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate buffer memory");
    }

    vkBindBufferMemory(device, _internal, _memoryInternal, 0);

    if(_type == UNIFORM)
    {
        vkMapMemory(device, _memoryInternal, 0, size, 0, &_dataPtr);
    }
}

void GraphicsBuffer::CopyToBuffer(const VkPhysicalDeviceHandler& physicalDevice, const VkDevice& device, const VkQueue& queue, const VkCommandPool& cmdPool, void* srcData, VkDeviceSize size)
{
    StageBuffer copyBuffer(physicalDevice, device, size);
    copyBuffer.CopyToBuffer(device, srcData, size);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = cmdPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmdBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmdBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    vkCmdCopyBuffer(cmdBuffer, copyBuffer._internal, _internal, 1, &copyRegion);

    vkEndCommandBuffer(cmdBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;

    // wait for the queue idle, we can use a fence to submit multiple shit later
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);
}

GraphicsBuffer::~GraphicsBuffer()
{
    if(_internal != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(_device, _internal, nullptr);
    }

    if(_memoryInternal != VK_NULL_HANDLE)
    {
        if(_dataPtr != nullptr)
        {
            vkUnmapMemory(_device, _memoryInternal);
            _dataPtr = nullptr;
        }
        vkFreeMemory(_device, _memoryInternal, nullptr);
    }

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "GraphicsBuffer Destroyed !");
#endif
}

GraphicsBuffer::GraphicsBuffer(GraphicsBuffer && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "GraphicsBuffer Move Creation !");
#endif

    _internal = other._internal;
    _memoryInternal = other._memoryInternal;
    _device = other._device;

    other._internal = VK_NULL_HANDLE;
    other._memoryInternal = VK_NULL_HANDLE;
}

GraphicsBuffer& GraphicsBuffer::operator=(GraphicsBuffer&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        _memoryInternal = other._memoryInternal;
        _device = other._device;
        _dataPtr = other._dataPtr;

        other._internal = VK_NULL_HANDLE;
        other._memoryInternal = VK_NULL_HANDLE;
        other._dataPtr = nullptr;
    }

    return *this;
}
