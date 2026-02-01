#pragma once

#include "header/GraphicsBuffer.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>
class CommandBuffer
{
    public:
    CommandBuffer(VkDevice device, VkCommandPool pool, VkQueue queue);

    void Begin(VkCommandBufferUsageFlags flags = 0) const;

    void End() const;

    void Submit(VkSemaphore waitSemaphores[], int waitCount,
        VkPipelineStageFlags waitStages[],
        VkSemaphore signalSemaphores[], int signalCount,
        VkFence fence = VK_NULL_HANDLE) const;

    // Todo: how to handle this properly (both need different params)
    void BindBuffer(const GraphicsBuffer& buffer) const;
    void SetScissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) const;
    void SetViewport(uint32_t width, uint32_t height, uint32_t x = 0, uint32_t y = 0, float minDepth = 1.0f, float maxDepth = 0.0f) const;
    void BindDescriptorSets(VkPipelineBindPoint bindPoint, const VkPipelineLayout& pipelineLayout, const VkDescriptorSet& set)
    {
        vkCmdBindDescriptorSets(_handle, bindPoint, pipelineLayout, 0, 1, &set, 0, nullptr);;
    }
    void DrawIndexed(uint32_t indexCount) const;
    void BindPipeline(VkPipeline pipeline, VkPipelineBindPoint bindPoint) const;
    void BeginRenderPass(VkRenderPass renderPass, VkFramebuffer framebuffer, VkClearColorValue clearColor, VkExtent2D extents, VkOffset2D offset = {0, 0}) const;
    void EndRenderPass() const;
    void Reset(VkCommandBufferResetFlags flags = 0);

    VkCommandBuffer GetInternal() const { return _handle; }

    private:
    VkDevice _device;
    VkCommandPool _pool;
    VkQueue _queue;
    VkCommandBuffer _handle;
};
