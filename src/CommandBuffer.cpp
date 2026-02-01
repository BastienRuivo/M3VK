#include "header/CommandBuffer.h"
#include "header/GraphicsBuffer.h"
#include <stdexcept>

CommandBuffer::CommandBuffer(VkDevice device, VkCommandPool pool, VkQueue queue) : _device(device), _pool(pool), _queue(queue)
{
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandPool = _pool;
    allocateInfo.commandBufferCount = 1;

    if(vkAllocateCommandBuffers(_device, &allocateInfo, &_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("Can't create main command buffer");
    }
}

void CommandBuffer::Begin(VkCommandBufferUsageFlags flags) const
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags; // Tells how we're using command buffer (record each send, buffer used in a render pass...)
    beginInfo.pInheritanceInfo = nullptr; // state info when called by a primary command buffer when it's a secondary one

    if(vkBeginCommandBuffer(_handle, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to begin command buffer !");
    }
}

void CommandBuffer::End() const
{
    if(vkEndCommandBuffer(_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to record command buffer");
    }
}

void CommandBuffer::Submit(VkSemaphore waitSemaphores[], int waitCount,
    VkPipelineStageFlags waitStages[],
    VkSemaphore signalSemaphores[], int signalCount,
    VkFence fence) const
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.waitSemaphoreCount = waitCount;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.signalSemaphoreCount = signalCount;
    submitInfo.pSignalSemaphores = signalSemaphores;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_handle;

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
            VkBuffer vertexBuffers[] = { buffer.GetInternal() };
            vkCmdBindVertexBuffers(_handle, 0, 1, vertexBuffers, offsets);
            break;
        }

        case GraphicsBuffer::INDEX:
        {
            vkCmdBindIndexBuffer(_handle, buffer.GetInternal(), 0, VK_INDEX_TYPE_UINT32);
            break;
        }

        case GraphicsBuffer::UNIFORM:
        {
            throw std::runtime_error("UNIMPLEMENTED");
        }
    }
}

void CommandBuffer::SetScissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) const
{
    VkRect2D scissors{};
    scissors.offset = {0, 0};
    scissors.extent = {width, height};

    vkCmdSetScissor(_handle, 0, 1, &scissors);
}

void CommandBuffer::SetViewport(uint32_t width, uint32_t height, uint32_t x, uint32_t y, float minDepth, float maxDepth) const
{
    VkViewport viewport{};
    viewport.x = x;
    viewport.y = y;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = minDepth;
    viewport.maxDepth = maxDepth;

    vkCmdSetViewport(_handle, 0, 1, &viewport);
}

void CommandBuffer::DrawIndexed(uint32_t indexCount) const
{
    vkCmdDrawIndexed(_handle, indexCount, 1, 0, 0, 0);
}

void CommandBuffer::BindPipeline(VkPipeline pipeline, VkPipelineBindPoint bindPoint) const
{
    vkCmdBindPipeline(_handle, bindPoint, pipeline);
}

void CommandBuffer::BeginRenderPass(VkRenderPass renderPass, VkFramebuffer framebuffer, VkClearColorValue clearColor, VkExtent2D extents, VkOffset2D offset) const
{
    VkRenderPassBeginInfo rpBeginInfo{};
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = renderPass;
    rpBeginInfo.framebuffer = framebuffer;
    rpBeginInfo.renderArea.offset = offset;
    rpBeginInfo.renderArea.extent = extents;

    VkClearValue clearValue{};
    clearValue.color = {0.0f, 0.0f, 0.0f, 0.0f};

    rpBeginInfo.clearValueCount = 1;
    rpBeginInfo.pClearValues = &clearValue;

    // inline tells everything is embedded in the primary cmdBuffer and no secondary will be used
    vkCmdBeginRenderPass(_handle, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::EndRenderPass() const
{
    vkCmdEndRenderPass(_handle);
}

void CommandBuffer::Reset(VkCommandBufferResetFlags flags)
{
    vkResetCommandBuffer(_handle, flags);
}
