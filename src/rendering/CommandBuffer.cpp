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

void CommandBuffer::Submit(VkSemaphore waitSemaphores[], int waitCount,
    VkPipelineStageFlags waitStages[],
    VkSemaphore signalSemaphores[], int signalCount,
    VkFence fence) const
{
    VkSubmitInfo submitInfo
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

        .waitSemaphoreCount = static_cast<uint32_t>(waitCount),
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,

        .commandBufferCount = 1,
        .pCommandBuffers = &_internal,

        .signalSemaphoreCount = static_cast<uint32_t>(signalCount),
        .pSignalSemaphores = signalSemaphores
    };

    if(vkQueueSubmit(_queue, 1, &submitInfo, fence) != VK_SUCCESS)
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

    vkCmdSetScissor(_internal, 0, 1, &scissors);
}

void CommandBuffer::SetScissor(const VkRect2D& scissors) const
{
    vkCmdSetScissor(_internal, 0, 1, &scissors);
}

void CommandBuffer::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) const
{
    VkViewport viewport
    {
        .x = (float)x,
        .y = (float)y,
        .width = (float)width,
        .height = (float)height,
        .minDepth = 0,
        .maxDepth = 1
    };

    vkCmdSetViewport(_internal, 0, 1, &viewport);
}

void CommandBuffer::WaitCompletion() const
{
    VkSubmitInfo submitInfo
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &_internal
    };

    vkQueueSubmit(_queue, 1, &submitInfo, VK_NULL_HANDLE);
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
