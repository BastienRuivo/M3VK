#include "rendering/CommandBuffer.h"
#include "application/ApplicationInfo.h"
#include "rendering/GraphicsBuffer.h"
#include "vulkan/vulkan.hpp"
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

CommandBuffer::CommandBuffer(vk::CommandPool pool, vk::Queue queue) : _pool(pool), _queue(queue)
{
    vk::CommandBufferAllocateInfo allocateInfo = vk::CommandBufferAllocateInfo{}
        .setCommandPool(_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1);

    if(ApplicationInfo::Device().allocateCommandBuffers(&allocateInfo, &_internal) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Can't create main command buffer");
    }
}

CommandBuffer::~CommandBuffer()
{
    if(!_internal) return;
    ApplicationInfo::Device().freeCommandBuffers(_pool, 1, &_internal);
}

void CommandBuffer::Begin(vk::CommandBufferUsageFlags flags) const
{
    vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo{}
        .setFlags(flags);

    if(_internal.begin(&beginInfo) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to begin command buffer !");
    }
}

void CommandBuffer::End() const
{
    _internal.end();
}

void CommandBuffer::Submit(std::span<const vk::SemaphoreSubmitInfo> waitSemaphores,
    std::span<const vk::SemaphoreSubmitInfo> signalSemaphores,
    vk::Fence fence) const
{
    vk::CommandBufferSubmitInfo commandBufferSubmitInfo = GetSubmitInfo();

    vk::SubmitInfo2 submitInfo = vk::SubmitInfo2{}
        .setWaitSemaphoreInfos(waitSemaphores)
        .setCommandBufferInfos(commandBufferSubmitInfo)
        .setSignalSemaphoreInfos(signalSemaphores);

    vk::Result result = _queue.submit2(1, &submitInfo, fence);
    if(result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to submit command queue with error " + vk::to_string(result));
    }
}

// Todo: how to handle this properly (both need different params)
void CommandBuffer::BindBuffer(const GraphicsBuffer& buffer) const
{
    vk::DeviceSize offsets[] = {0};

    switch (buffer.GetType())
    {
        case GraphicsBuffer::VERTEX:
        {
            vk::Buffer vertexBuffers[] = { buffer.Internal() };
            _internal.bindVertexBuffers(0, 1, vertexBuffers, offsets);
            break;
        }

        case GraphicsBuffer::INDEX:
        {
            _internal.bindIndexBuffer(buffer.Internal(), 0, vk::IndexType::eUint32);
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
    vk::Rect2D scissors = vk::Rect2D{}
        .setOffset({x, y})
        .setExtent({width, height});
    SetScissor(scissors);
}

void CommandBuffer::SetScissor(const vk::Rect2D& scissors) const
{
    _internal.setScissorWithCount(scissors);
}

void CommandBuffer::SetViewport(float x, float y, float width, float height) const
{
    vk::Viewport viewport = vk::Viewport{}
        .setX(x)
        .setY(y)
        .setWidth(width)
        .setHeight(height)
        .setMinDepth(0.0f)
        .setMaxDepth(1.0f);

    _internal.setViewportWithCount(1, &viewport);
}

void CommandBuffer::WaitCompletion() const
{
    vk::raii::Fence waitFence(ApplicationInfo::RaiiDevice(), vk::FenceCreateInfo{});
    vk::CommandBufferSubmitInfo commandBufferSubmitInfo = GetSubmitInfo();

    vk::SubmitInfo2 submitInfo = vk::SubmitInfo2{}
        .setCommandBufferInfos(commandBufferSubmitInfo);

    vk::Result result = _queue.submit2(1, &submitInfo, waitFence);
    if(result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to submit command buffer (CopyBufferToBuffer)");
    }

    result = ApplicationInfo::RaiiDevice().waitForFences(*waitFence, vk::True, UINT64_MAX);
    if(result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Wait for single command buffer failed !");
    }
}

CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept
: _pool(other._pool), _queue(other._queue), _internal(other._internal)
{
    other._internal = nullptr;
}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& other) noexcept
{
    if(this != &other)
    {

        _internal = other._internal;
        _pool = other._pool;
        _queue = other._queue;
        other._internal = nullptr;
    }
    return *this;
}
