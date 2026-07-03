#pragma once

#include "allocation/DescriptorPool.h"
#include "rendering/GPUImage.h"
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

    void Submit(std::span<const VkSemaphoreSubmitInfo> waitSemaphores,
    std::span<const VkSemaphoreSubmitInfo> signalSemaphores,
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

    inline void Barrier(std::span<const VkMemoryBarrier2> memoryBarriers) const
    {
        VkDependencyInfo dependencyInfo
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .dependencyFlags = 0,
            .memoryBarrierCount = static_cast<uint32_t>(memoryBarriers.size()),
            .pMemoryBarriers = memoryBarriers.data()
        };
        vkCmdPipelineBarrier2(_internal, &dependencyInfo);
    }

    inline void Barrier(std::span<const VkBufferMemoryBarrier2> bufferBarriers) const
    {
        VkDependencyInfo dependencyInfo
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .dependencyFlags = 0,
            .bufferMemoryBarrierCount = static_cast<uint32_t>(bufferBarriers.size()),
            .pBufferMemoryBarriers = bufferBarriers.data()
        };
        vkCmdPipelineBarrier2(_internal, &dependencyInfo);
    }

    inline void Barrier(std::span<const VkImageMemoryBarrier2> imgBarriers) const
    {
        VkDependencyInfo dependencyInfo
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .dependencyFlags = 0,
            .imageMemoryBarrierCount = static_cast<uint32_t>(imgBarriers.size()),
            .pImageMemoryBarriers = imgBarriers.data()
        };
        vkCmdPipelineBarrier2(_internal, &dependencyInfo);
    }

    inline void Barrier(std::span<const VkMemoryBarrier2> memoryBarriers, std::span<const VkBufferMemoryBarrier2> bufferBarriers, std::span<const VkImageMemoryBarrier2> imgBarriers) const
    {
        VkDependencyInfo dependencyInfo
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .dependencyFlags = 0,
            .memoryBarrierCount = static_cast<uint32_t>(memoryBarriers.size()),
            .pMemoryBarriers = memoryBarriers.data(),
            .bufferMemoryBarrierCount = static_cast<uint32_t>(bufferBarriers.size()),
            .pBufferMemoryBarriers = bufferBarriers.data(),
            .imageMemoryBarrierCount = static_cast<uint32_t>(imgBarriers.size()),
            .pImageMemoryBarriers = imgBarriers.data()
        };
        vkCmdPipelineBarrier2(_internal, &dependencyInfo);
    }

    inline void Blit(VkImage src, VkImageLayout srcLayout, VkImage dst, VkImageLayout dstLayout, std::span<const VkImageBlit> regions, VkFilter filter) const
    {
        vkCmdBlitImage(_internal, src, srcLayout, dst, dstLayout, regions.size(), regions.data(), filter);
    }

    inline void Blit(ImageReference src, VkImageLayout srcLayout, ImageReference dst, VkImageLayout dstLayout, VkFilter filter = VK_FILTER_NEAREST) const
    {
        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        if(src.UsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        VkImageBlit region
        {
            .srcSubresource = { aspect, 0, 0, src.ArrayLayerCount },
            .srcOffsets = { { 0, 0, 0 }, { static_cast<int32_t>(src.Width), static_cast<int32_t>(src.Height), 1 } },
            .dstSubresource = { aspect, 0, 0, dst.ArrayLayerCount },
            .dstOffsets = { { 0, 0, 0 }, { static_cast<int32_t>(dst.Width), static_cast<int32_t>(dst.Height), 1 } }
        };
        vkCmdBlitImage(_internal, src.Image, srcLayout, dst.Image, dstLayout, 1, &region, filter);
    }

    inline void CopyImage(ImageReference src, VkImageLayout srcLayout, ImageReference dst, VkImageLayout dstLayout) const
    {
        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        if(src.UsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) aspect = VK_IMAGE_ASPECT_DEPTH_BIT;

        VkImageCopy2 region
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
            .srcSubresource = { aspect, 0, 0, src.ArrayLayerCount },
            .srcOffset = { 0, 0, 0 },
            .dstSubresource = { aspect, 0, 0, dst.ArrayLayerCount },
            .dstOffset = { 0, 0, 0 },
            .extent = { static_cast<uint32_t>(src.Width), static_cast<uint32_t>(src.Height), 1 }
        };
        VkCopyImageInfo2 copyInfo
        {
            .sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
            .srcImage = src.Image,
            .srcImageLayout = srcLayout,
            .dstImage = dst.Image,
            .dstImageLayout = dstLayout,
            .regionCount = 1,
            .pRegions = &region
        };
        vkCmdCopyImage2(_internal, &copyInfo);
    }

    inline void CopyBufferToImage(VkBuffer buffer, VkImage image, VkImageLayout layout, std::span<const VkBufferImageCopy> regions) const
    {
        vkCmdCopyBufferToImage(_internal, buffer, image, layout, static_cast<uint32_t>(regions.size()), regions.data());
    }

    inline void CopyImageToBuffer(VkImage image, VkImageLayout layout, VkBuffer buffer, std::span<const VkBufferImageCopy> regions) const
    {
        vkCmdCopyImageToBuffer(_internal, image, layout, buffer, static_cast<uint32_t>(regions.size()), regions.data());
    }

    inline void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, std::span<const VkBufferCopy> pRegions) const
    {
        vkCmdCopyBuffer(_internal, src, dst, static_cast<uint32_t>(pRegions.size()), pRegions.data());
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

    inline void BeginRendering(VkRect2D renderArea, std::span<const VkRenderingAttachmentInfo> colorAttachments, const VkRenderingAttachmentInfo& depthAttachment, const VkRenderingAttachmentInfo& stencilAttachment) const
    {
        VkRenderingInfo renderingInfo
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = renderArea,
            .layerCount = 1,
            .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
            .pColorAttachments = colorAttachments.data(),
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

    inline void SetVertexInput(std::span<const VkVertexInputBindingDescription2EXT> vertexBindingDescriptions, std::span<const VkVertexInputAttributeDescription2EXT> vertexAttributeDescriptions) const
    {
        VkFunctions::vkCmdSetVertexInputEXT(_internal, static_cast<uint32_t>(vertexBindingDescriptions.size()), vertexBindingDescriptions.data(), static_cast<uint32_t>(vertexAttributeDescriptions.size()), vertexAttributeDescriptions.data());
    }

    inline void BindShaders(std::span<const VkShaderStageFlagBits> stages, const VkShaderEXT* pShaders) const
    {
        VkFunctions::vkCmdBindShadersEXT(_internal, static_cast<uint32_t>(stages.size()), stages.data(), pShaders);
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

    inline VkCommandBufferSubmitInfo GetSubmitInfo() const
    {
        return VkCommandBufferSubmitInfo
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = _internal
        };
    }

    inline VkCommandBuffer GetInternal() const { return _internal; }

    private:

    void CreateSingleTime();
    void CreateMultiUsage();

    VkCommandPool _pool = VK_NULL_HANDLE;
    VkQueue _queue = VK_NULL_HANDLE;
    VkCommandBuffer _internal = VK_NULL_HANDLE;
};
