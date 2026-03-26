#pragma once

#include "header/GraphicsBuffer.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>
class CommandBuffer
{
    public:
    enum Usage
    {
        SingleTime,
        MultipleTime
    };

    CommandBuffer(VkDevice device, VkCommandPool pool, VkQueue queue);
    ~CommandBuffer();

    CommandBuffer(CommandBuffer&& other) noexcept;
    CommandBuffer& operator=(CommandBuffer&& other) noexcept;

    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;

    void Begin(VkCommandBufferUsageFlags flags = 0) const;

    void End() const;

    void Submit(VkSemaphore waitSemaphores[], int waitCount,
        VkPipelineStageFlags waitStages[],
        VkSemaphore signalSemaphores[], int signalCount,
        VkFence fence = VK_NULL_HANDLE) const;

    inline void BeginSingleTime()
    {
        Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    }

    void WaitCompletion();

    // Todo: how to handle this properly (both need different params)
    void BindBuffer(const GraphicsBuffer& buffer) const;
    void SetScissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) const;
    void SetViewport(uint32_t width, uint32_t height, uint32_t x = 0, uint32_t y = 0, float minDepth = 1.0f, float maxDepth = 0.0f) const;
    void BindDescriptorSets(VkPipelineBindPoint bindPoint, const VkPipelineLayout& pipelineLayout, const VkDescriptorSet& set) const
    {
        vkCmdBindDescriptorSets(_internal, bindPoint, pipelineLayout, 0, 1, &set, 0, nullptr);;
    }
    inline void DrawIndexed(uint32_t indexCount) const
    {
        vkCmdDrawIndexed(_internal, indexCount, 1, 0, 0, 0);
    }

    inline void BindPipeline(VkPipeline pipeline, VkPipelineBindPoint bindPoint) const
    {
        vkCmdBindPipeline(_internal, bindPoint, pipeline);
    }

    inline void EndRenderPass() const
    {
        vkCmdEndRenderPass(_internal);
    }

    void BeginRenderPass(VkRenderPass renderPass, VkFramebuffer framebuffer, VkClearColorValue clearColor, VkExtent2D extents, VkOffset2D offset = {0, 0}) const;

    void Reset(VkCommandBufferResetFlags flags = 0) const
    {
        vkResetCommandBuffer(_internal, flags);
    }

    inline void Barrier(VkMemoryBarrier* memoryBarriers, uint32_t memoryBarrierCount, VkBufferMemoryBarrier* bufferBarriers, uint32_t bufferBarrierCount, VkImageMemoryBarrier* imgBarriers, uint32_t imgBarrierCount) const
    {
        vkCmdPipelineBarrier(_internal,
            0, // todo
            0, // todo
            0,
            memoryBarrierCount, memoryBarriers,
            bufferBarrierCount, bufferBarriers,
            imgBarrierCount, imgBarriers
        );
    }

    inline void CopyBufferToImage(VkBuffer buffer, VkImage image, VkImageLayout layout, VkBufferImageCopy* pRegions, int regionCount)
    {
        vkCmdCopyBufferToImage(_internal, buffer, image, layout, regionCount, pRegions);
    }

    inline VkCommandBuffer GetInternal() const { return _internal; }

    private:

    void CreateSingleTime();
    void CreateMultiUsage();

    VkDevice _device;
    VkCommandPool _pool;
    VkQueue _queue;
    VkCommandBuffer _internal;
};
