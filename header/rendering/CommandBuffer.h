#pragma once

#include "rendering/DescriptorPool.h"
#include "rendering/GraphicsBuffer.h"
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

    CommandBuffer(VkCommandPool pool, VkQueue queue);
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

    inline void BeginSingleTime() const
    {
        Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    }

    void WaitCompletion() const;

    // Todo: how to handle this properly (both need different params)
    void BindBuffer(const GraphicsBuffer& buffer) const;
    void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) const;
    void SetScissor(const VkRect2D& scissors) const;
    void SetViewport(float x, float y, float width, float height) const;
    void TransitionImageLayout(VkImage img, VkFormat format, uint32_t mipLevel, uint32_t mipCount, VkImageLayout oldLayout, VkImageLayout newLayout) const;

    inline void BindDescriptorSets(const VkPipelineLayout& pipelineLayout, const DescriptorSetHandle& setHandle, VkPipelineBindPoint bindPoint, uint32_t location) const
    {
        vkCmdBindDescriptorSets(_internal, bindPoint, pipelineLayout, location, 1, &setHandle.Set, 0, nullptr);
    }

    inline void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
    {
        vkCmdDraw(_internal, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    inline void DrawIndexed(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexOffset) const
    {
        vkCmdDrawIndexed(_internal, indexCount, 1, firstIndex, vertexOffset, 0);
    }

    inline void DrawIndexedIndirect(VkBuffer indirectBuffer, VkDeviceSize offsetInBytes, uint32_t drawCount, uint32_t stride) const
    {
        vkCmdDrawIndexedIndirect(_internal, indirectBuffer, offsetInBytes, drawCount, stride);
    }

    inline void BindPipeline(VkPipeline pipeline, VkPipelineBindPoint bindPoint) const
    {
        vkCmdBindPipeline(_internal, bindPoint, pipeline);
    }

    void Reset(VkCommandBufferResetFlags flags = 0) const
    {
        vkResetCommandBuffer(_internal, flags);
    }

    inline void PushConstants(VkPipelineLayout layout, uint32_t offset, uint32_t size, const void* pValues) const
    {
        vkCmdPushConstants(_internal, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, offset, size, pValues);
    }

    inline void Barrier(VkPipelineStageFlags srcAccesMask, VkPipelineStageFlags dstAccesMask, VkMemoryBarrier* memoryBarriers, uint32_t memoryBarrierCount, VkBufferMemoryBarrier* bufferBarriers, uint32_t bufferBarrierCount, VkImageMemoryBarrier* imgBarriers, uint32_t imgBarrierCount) const
    {
        vkCmdPipelineBarrier(_internal,
            srcAccesMask, dstAccesMask,
            0,
            memoryBarrierCount, memoryBarriers,
            bufferBarrierCount, bufferBarriers,
            imgBarrierCount, imgBarriers
        );
    }

    inline void Blit(VkImage src, VkImageLayout srcLayout, VkImage dst, VkImageLayout dstLayout, uint32_t regionCount, VkImageBlit* pRegions, VkFilter filter) const
    {
        vkCmdBlitImage(_internal, src, srcLayout, dst, dstLayout, regionCount, pRegions, filter);
    }

    inline void CopyBufferToImage(VkBuffer buffer, VkImage image, VkImageLayout layout, VkBufferImageCopy* pRegions, int regionCount) const
    {
        vkCmdCopyBufferToImage(_internal, buffer, image, layout, regionCount, pRegions);
    }

    inline void CopyImageToBuffer(VkImage image, VkImageLayout layout, VkBuffer buffer, VkBufferImageCopy* pRegions, int regionCount) const
    {
        vkCmdCopyImageToBuffer(_internal, image, layout, buffer, regionCount, pRegions);
    }

    inline void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkBufferCopy* pRegions, int regionCount)
    {
        vkCmdCopyBuffer(_internal, src, dst, regionCount, pRegions);
    }

    inline void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, uint32_t srcIndexInBytes = 0, uint32_t dstIndexInBytes = 0) const
    {
        VkBufferCopy region
        {
            .srcOffset = srcIndexInBytes,
            .dstOffset = dstIndexInBytes,
            .size = size
        };

        vkCmdCopyBuffer(_internal, src, dst, 1, &region);
    }

    inline void BeginRendering(VkRect2D renderArea, const VkRenderingAttachmentInfo * colorAttachment, uint32_t colorAttachmentCount, const VkRenderingAttachmentInfo& depthAttachment, const VkRenderingAttachmentInfo& stencilAttachment) const
    {
        VkRenderingInfo renderingInfo
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = renderArea,
            .layerCount = 1,
            .colorAttachmentCount = colorAttachmentCount,
            .pColorAttachments = colorAttachment,
            .pDepthAttachment = &depthAttachment,
            .pStencilAttachment = &stencilAttachment
        };
        vkCmdBeginRendering(_internal, &renderingInfo);
    }

    inline void EndRendering() const
    {
        vkCmdEndRendering(_internal);
    }

    inline void SetDepthCompareOp(VkCompareOp op) const
    {
        vkCmdSetDepthCompareOp(_internal, op);
    }

    inline void SetDepthTest(VkBool32 enable) const
    {
        vkCmdSetDepthTestEnable(_internal, enable);
    }

    inline void SetDepthWrite(VkBool32 enable) const
    {
        vkCmdSetDepthWriteEnable(_internal, enable);
    }

    inline void SetStencilTest(VkBool32 enable) const
    {
        vkCmdSetStencilTestEnable(_internal, enable);
    }

    inline void SetBlendEnable(VkBool32 enable) const
    {
        VkFunctions::vkCmdSetColorBlendEnableEXT(_internal, 0, 1, &enable);
    }

    inline void SetBlendEquation(VkColorBlendEquationEXT equation) const
    {
        VkFunctions::vkCmdSetColorBlendEquationEXT(_internal, 0, 1, &equation);
    }

    inline void SetColorWriteMask(VkColorComponentFlags mask) const
    {
        VkFunctions::vkCmdSetColorWriteMaskEXT(_internal, 0, 1, &mask);
    }

    inline void SetPrimitiveTopology(VkPrimitiveTopology topology) const
    {
        vkCmdSetPrimitiveTopology(_internal, topology);
    }

    inline void SetPrimitiveRestart(VkBool32 enable) const
    {
        vkCmdSetPrimitiveRestartEnable(_internal, enable);
    }

    inline void SetRasterizerDiscard(VkBool32 enable) const
    {
        vkCmdSetRasterizerDiscardEnable(_internal, enable);
    }

    inline void SetCullMode(VkCullModeFlags cullMode) const
    {
        vkCmdSetCullMode(_internal, cullMode);
    }

    inline void SetFrontFace(VkFrontFace frontFace) const
    {
        vkCmdSetFrontFace(_internal, frontFace);
    }

    inline void SetPolygonMode(VkPolygonMode polygonMode) const
    {
        VkFunctions::vkCmdSetPolygonModeEXT(_internal, polygonMode);
    }

    inline void SetLineWidth(float lineWidth) const
    {
        vkCmdSetLineWidth(_internal, lineWidth);
    }

    inline void SetDepthBiasEnable(VkBool32 enable) const
    {
        vkCmdSetDepthBiasEnable(_internal, enable);
    }

    inline void SetRasterizationSamples(VkSampleCountFlagBits samples) const
    {
        VkFunctions::vkCmdSetRasterizationSamplesEXT(_internal, samples);
    }

    inline void SetSampleMask(VkSampleCountFlagBits samples, VkSampleMask sampleMask) const
    {
        VkFunctions::vkCmdSetSampleMaskEXT(_internal, samples, &sampleMask);
    }

    inline void SetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) const
    {
        vkCmdSetDepthBias(_internal, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
    }

    inline void SetSampleMask(VkSampleCountFlagBits samples, const VkSampleMask* sampleMask)
    {
        VkFunctions::vkCmdSetSampleMaskEXT(_internal, samples, sampleMask);
    }

    inline void SetAlphaToCoverageEnable(VkBool32 enable) const
    {
        VkFunctions::vkCmdSetAlphaToCoverageEnableEXT(_internal, enable);
    }

    inline void SetAlphaToOneEnable(VkBool32 enable) const
    {
        VkFunctions::vkCmdSetAlphaToOneEnableEXT(_internal, enable);
    }

    inline void SetDepthClampEnable(VkBool32 enable) const
    {
        VkFunctions::vkCmdSetDepthClampEnableEXT(_internal, enable);
    }

    inline void SetVertexInput(uint32_t bindingCount, const VkVertexInputBindingDescription2EXT* pVertexBindingDescriptions, uint32_t vertexAttributeDescriptionCount, const VkVertexInputAttributeDescription2EXT* pVertexAttributeDescriptions) const
    {
        VkFunctions::vkCmdSetVertexInputEXT(_internal, bindingCount, pVertexBindingDescriptions, vertexAttributeDescriptionCount, pVertexAttributeDescriptions);
    }

    inline void BindShaders(uint32_t stageCount, const VkShaderStageFlagBits* pStages, const VkShaderEXT* pShaders) const
    {
        VkFunctions::vkCmdBindShadersEXT(_internal, stageCount, pStages, pShaders);
    }

    inline void Dispatch(uint32_t x, uint32_t y, uint32_t z) const
    {
        vkCmdDispatch(_internal, x, y, z);
    }

    inline void BeginMarker(const char* name) const
    {
        VkDebugUtilsLabelEXT label{};
        label.pLabelName = name;
        label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        VkFunctions::vkCmdBeginDebugUtilsLabelEXT(_internal, &label);
    }

    inline void EndMarker() const
    {
        VkFunctions::vkCmdEndDebugUtilsLabelEXT(_internal);
    }

    inline VkCommandBuffer GetInternal() const { return _internal; }

    private:

    void CreateSingleTime();
    void CreateMultiUsage();

    VkCommandPool _pool = VK_NULL_HANDLE;
    VkQueue _queue = VK_NULL_HANDLE;
    VkCommandBuffer _internal = VK_NULL_HANDLE;
};
