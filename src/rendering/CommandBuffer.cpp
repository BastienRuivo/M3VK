#include "rendering/CommandBuffer.h"
#include "application/ApplicationInfo.h"
#include "rendering/GraphicsBuffer.h"
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

CommandBuffer::CommandBuffer(VkCommandPool pool, VkQueue queue) : _pool(pool), _queue(queue)
{
    VkCommandBufferAllocateInfo allocateInfo
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = _pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    if(vkAllocateCommandBuffers(ApplicationInfo::Device(), &allocateInfo, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Can't create main command buffer");
    }
}

CommandBuffer::~CommandBuffer()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkFreeCommandBuffers(ApplicationInfo::Device(), _pool, 1, &_internal);
}

void CommandBuffer::Begin(VkCommandBufferUsageFlags flags) const
{
    VkCommandBufferBeginInfo beginInfo
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = flags,
        .pInheritanceInfo = nullptr
    };

    if(vkBeginCommandBuffer(_internal, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to begin command buffer !");
    }
}

void CommandBuffer::End() const
{
    if(vkEndCommandBuffer(_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to record command buffer");
    }
}

void CommandBuffer::Submit(std::span<const VkSemaphoreSubmitInfo> waitSemaphores,
    std::span<const VkSemaphoreSubmitInfo> signalSemaphores,
    VkFence fence) const
{
    VkCommandBufferSubmitInfo commandBufferSubmitInfo = GetSubmitInfo();

    VkSubmitInfo2 submitInfo
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = static_cast<uint32_t>(waitSemaphores.size()),
        .pWaitSemaphoreInfos = waitSemaphores.data(),
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &commandBufferSubmitInfo,
        .signalSemaphoreInfoCount = static_cast<uint32_t>(signalSemaphores.size()),
        .pSignalSemaphoreInfos = signalSemaphores.data(),
    };

    if(vkQueueSubmit2(_queue, 1, &submitInfo, fence) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command");
    }
}

// Todo: how to handle this properly (both need different params)
void CommandBuffer::BindBuffer(const GraphicsBuffer& buffer) const
{
    VkDeviceSize offsets[] = {0};

    switch (buffer.GetType())
    {
        case GraphicsBuffer::VERTEX:
        {
            VkBuffer vertexBuffers[] = { buffer.Internal() };
            vkCmdBindVertexBuffers(_internal, 0, 1, vertexBuffers, offsets);
            break;
        }

        case GraphicsBuffer::INDEX:
        {
            vkCmdBindIndexBuffer(_internal, buffer.Internal(), 0, VK_INDEX_TYPE_UINT32);
            break;
        }

        default:
        {
            throw std::runtime_error("Unknown buffer type to bind : " + std::to_string(buffer.GetType()));
        }
    }
}

void CommandBuffer::SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) const
{
    VkRect2D scissors
    {
        .offset = {x, y},
        .extent = {width, height}
    };
    SetScissor(scissors);
}

void CommandBuffer::SetScissor(const VkRect2D& scissors) const
{
    vkCmdSetScissorWithCount(_internal, 1, &scissors);
}

void CommandBuffer::SetViewport(float x, float y, float width, float height) const
{
    VkViewport viewport
    {
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .minDepth = 0,
        .maxDepth = 1
    };

    vkCmdSetViewportWithCount(_internal, 1, &viewport);
}

void CommandBuffer::WaitCompletion() const
{
    VkCommandBufferSubmitInfo commandBufferSubmitInfo = GetSubmitInfo();

    VkSubmitInfo2 submitInfo
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &commandBufferSubmitInfo
    };

    vkQueueSubmit2(_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(_queue);
}

CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept
: _pool(other._pool), _queue(other._queue), _internal(other._internal)
{
    other._internal = VK_NULL_HANDLE;
}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& other) noexcept
{
    if(this != &other)
    {

        _internal = other._internal;
        _pool = other._pool;
        _queue = other._queue;
        other._internal = VK_NULL_HANDLE;
    }
    return *this;
}
