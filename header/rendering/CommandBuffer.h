#pragma once

#include "allocation/DescriptorPool.h"
#include "rendering/GPUImage.h"
#include "rendering/GraphicsBuffer.h"
#include <cstdint>
#include <vulkan/vulkan.hpp>
class CommandBuffer
{
    public:
    enum Usage
    {
        SingleTime,
        MultipleTime
    };

    CommandBuffer(vk::CommandPool pool, vk::Queue queue);
    ~CommandBuffer();

    CommandBuffer(CommandBuffer&& other) noexcept;
    CommandBuffer& operator=(CommandBuffer&& other) noexcept;

    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;

    void Begin(vk::CommandBufferUsageFlags flags = {}) const;

    void End() const;

    void Submit(std::span<const vk::SemaphoreSubmitInfo> waitSemaphores,
    std::span<const vk::SemaphoreSubmitInfo> signalSemaphores, vk::Fence fence = nullptr) const;

    inline void BeginSingleTime() const
    {
        Begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    }

    void WaitCompletion() const;

    // Todo: how to handle this properly (both need different params)
    void BindBuffer(const GraphicsBuffer& buffer) const;
    void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) const;
    void SetScissor(const vk::Rect2D& scissors) const;
    void SetViewport(float x, float y, float width, float height) const;

    inline void BindDescriptorSets(const vk::PipelineLayout& pipelineLayout, const DescriptorSetHandle& setHandle, vk::PipelineBindPoint bindPoint, uint32_t location) const
    {
        _internal.bindDescriptorSets(bindPoint, pipelineLayout, location, 1, &setHandle.Set, 0, nullptr);
    }

    inline void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
    {
        _internal.draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    inline void DrawIndexed(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexOffset) const
    {
        _internal.drawIndexed(indexCount, 1, firstIndex, vertexOffset, 0);
    }

    inline void DrawIndexedIndirect(vk::Buffer indirectBuffer, vk::DeviceSize offsetInBytes, uint32_t drawCount, uint32_t stride) const
    {
        _internal.drawIndexedIndirect(indirectBuffer, offsetInBytes, drawCount, stride);
    }

    inline void BindPipeline(vk::Pipeline pipeline, vk::PipelineBindPoint bindPoint) const
    {
        _internal.bindPipeline(bindPoint, pipeline);
    }

    void Reset(vk::CommandBufferResetFlags flags = {}) const
    {
        _internal.reset(flags);
    }

    inline void PushConstants(vk::PipelineLayout layout, uint32_t offset, uint32_t size, const void* pValues) const
    {
        _internal.pushConstants(layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute, offset, size, pValues);
    }

    inline void Barrier(std::span<const vk::MemoryBarrier2> memoryBarriers) const
    {
        vk::DependencyInfo dependencyInfo = vk::DependencyInfo{}
            .setMemoryBarriers(memoryBarriers);
        _internal.pipelineBarrier2(dependencyInfo);
    }

    inline void Barrier(std::span<const vk::BufferMemoryBarrier2> bufferBarriers) const
    {
        vk::DependencyInfo dependencyInfo = vk::DependencyInfo{}
            .setBufferMemoryBarriers(bufferBarriers);
        _internal.pipelineBarrier2(dependencyInfo);
    }

    inline void Barrier(std::span<const vk::ImageMemoryBarrier2> imgBarriers) const
    {
        vk::DependencyInfo dependencyInfo = vk::DependencyInfo{}
            .setImageMemoryBarriers(imgBarriers);
        _internal.pipelineBarrier2(dependencyInfo);
    }

    inline void Barrier(std::span<const vk::MemoryBarrier2> memoryBarriers, std::span<const vk::BufferMemoryBarrier2> bufferBarriers, std::span<const vk::ImageMemoryBarrier2> imgBarriers) const
    {
        vk::DependencyInfo dependencyInfo = vk::DependencyInfo{}
            .setMemoryBarriers(memoryBarriers)
            .setBufferMemoryBarriers(bufferBarriers)
            .setImageMemoryBarriers(imgBarriers);
        _internal.pipelineBarrier2(dependencyInfo);
    }

    inline void Blit(vk::Image src, vk::ImageLayout srcLayout, vk::Image dst, vk::ImageLayout dstLayout, std::span<const vk::ImageBlit> regions, vk::Filter filter) const
    {
        _internal.blitImage(src, srcLayout, dst, dstLayout, regions, filter);
    }

    inline void Blit(ImageReference src, vk::ImageLayout srcLayout, ImageReference dst, vk::ImageLayout dstLayout, vk::Filter filter = vk::Filter::eNearest) const
    {
        vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor;
        if(src.UsageFlags & vk::ImageUsageFlagBits::eDepthStencilAttachment) aspect = vk::ImageAspectFlagBits::eDepth;
        vk::ImageBlit region = vk::ImageBlit{}
            .setSrcSubresource(vk::ImageSubresourceLayers{aspect, 0, 0, src.ArrayLayerCount})
            .setSrcOffsets({vk::Offset3D{0, 0, 0}, vk::Offset3D{static_cast<int32_t>(src.Width), static_cast<int32_t>(src.Height), 1}})
            .setDstSubresource(vk::ImageSubresourceLayers{aspect, 0, 0, dst.ArrayLayerCount})
            .setDstOffsets({vk::Offset3D{0, 0, 0}, vk::Offset3D{static_cast<int32_t>(dst.Width), static_cast<int32_t>(dst.Height), 1}});
        _internal.blitImage(src.Image, srcLayout, dst.Image, dstLayout, region, filter);
    }

    inline void CopyImage(ImageReference src, vk::ImageLayout srcLayout, ImageReference dst, vk::ImageLayout dstLayout) const
    {
        vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor;
        if(src.UsageFlags & vk::ImageUsageFlagBits::eDepthStencilAttachment) aspect = vk::ImageAspectFlagBits::eDepth;

        vk::ImageCopy2 region = vk::ImageCopy2{}
            .setSrcSubresource(vk::ImageSubresourceLayers{aspect, 0, 0, src.ArrayLayerCount})
            .setSrcOffset(vk::Offset3D{0, 0, 0})
            .setDstSubresource(vk::ImageSubresourceLayers{aspect, 0, 0, dst.ArrayLayerCount})
            .setDstOffset(vk::Offset3D{0, 0, 0})
            .setExtent(vk::Extent3D{static_cast<uint32_t>(src.Width), static_cast<uint32_t>(src.Height), 1});

        vk::CopyImageInfo2 copyInfo = vk::CopyImageInfo2{}
            .setSrcImage(src.Image)
            .setSrcImageLayout(srcLayout)
            .setDstImage(dst.Image)
            .setDstImageLayout(dstLayout)
            .setRegions(region);

        _internal.copyImage2(copyInfo);
    }

    inline void CopyBufferToImage(vk::Buffer buffer, vk::Image image, vk::ImageLayout layout, std::span<const vk::BufferImageCopy> regions) const
    {
        _internal.copyBufferToImage(buffer, image, layout, regions);
    }

    inline void CopyImageToBuffer(vk::Image image, vk::ImageLayout layout, vk::Buffer buffer, std::span<const vk::BufferImageCopy> regions) const
    {
        _internal.copyImageToBuffer(image, layout, buffer, regions);
    }

    inline void CopyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size, std::span<const vk::BufferCopy> pRegions) const
    {
        _internal.copyBuffer(src, dst, pRegions);
    }

    inline void CopyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size, uint32_t srcIndexInBytes = 0, uint32_t dstIndexInBytes = 0) const
    {
        vk::BufferCopy region = vk::BufferCopy{}
            .setSrcOffset(srcIndexInBytes)
            .setDstOffset(dstIndexInBytes)
            .setSize(size);

        _internal.copyBuffer(src, dst, region);
    }

    inline void BeginRendering(vk::Rect2D renderArea, std::span<const vk::RenderingAttachmentInfo> colorAttachments, const vk::RenderingAttachmentInfo& depthAttachment, const vk::RenderingAttachmentInfo& stencilAttachment) const
    {
        vk::RenderingInfo renderingInfo = vk::RenderingInfo{}
            .setRenderArea(renderArea)
            .setLayerCount(1)
            .setColorAttachments(colorAttachments)
            .setPDepthAttachment(&depthAttachment)
            .setPStencilAttachment(&stencilAttachment);
        _internal.beginRendering(renderingInfo);
    }

    inline void EndRendering() const
    {
        _internal.endRendering();
    }

    inline void SetDepthCompareOp(vk::CompareOp op) const
    {
        _internal.setDepthCompareOp(op);
    }

    inline void SetDepthTest(vk::Bool32 enable) const
    {
        _internal.setDepthTestEnable(enable);
    }

    inline void SetDepthWrite(vk::Bool32 enable) const
    {
        _internal.setDepthWriteEnable(enable);
    }

    inline void SetStencilTest(vk::Bool32 enable) const
    {
        _internal.setStencilTestEnable(enable);
    }

    inline void SetBlendEnable(vk::Bool32 enable) const
    {
        vk::CommandBuffer(_internal).setColorBlendEnableEXT(0, enable);
    }

    inline void SetBlendEquation(vk::ColorBlendEquationEXT equation) const
    {
        vk::CommandBuffer(_internal).setColorBlendEquationEXT(0, equation);
    }

    inline void SetColorWriteMask(vk::ColorComponentFlags mask) const
    {
        vk::CommandBuffer(_internal).setColorWriteMaskEXT(0, mask);
    }

    inline void SetPrimitiveTopology(vk::PrimitiveTopology topology) const
    {
        _internal.setPrimitiveTopology(topology);
    }

    inline void SetPrimitiveRestart(vk::Bool32 enable) const
    {
        _internal.setPrimitiveRestartEnable(enable);
    }

    inline void SetRasterizerDiscard(vk::Bool32 enable) const
    {
        _internal.setRasterizerDiscardEnable(enable);
    }

    inline void SetCullMode(vk::CullModeFlags cullMode) const
    {
        vk::CommandBuffer(_internal).setCullMode(cullMode);
    }

    inline void SetFrontFace(vk::FrontFace frontFace) const
    {
        vk::CommandBuffer(_internal).setFrontFace(frontFace);
    }

    inline void SetPolygonMode(vk::PolygonMode polygonMode) const
    {
        vk::CommandBuffer(_internal).setPolygonModeEXT(polygonMode);
    }

    inline void SetLineWidth(float lineWidth) const
    {
        vk::CommandBuffer(_internal).setLineWidth(lineWidth);
    }

    inline void SetDepthBiasEnable(vk::Bool32 enable) const
    {
        vk::CommandBuffer(_internal).setDepthBiasEnable(enable);
    }

    inline void SetRasterizationSamples(vk::SampleCountFlagBits samples) const
    {
        vk::CommandBuffer(_internal).setRasterizationSamplesEXT(samples);
    }

    inline void SetSampleMask(vk::SampleCountFlagBits samples, vk::SampleMask sampleMask) const
    {
        vk::CommandBuffer(_internal).setSampleMaskEXT(samples, &sampleMask);
    }

    inline void SetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) const
    {
        _internal.setDepthBias(depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
    }

    inline void SetSampleMask(vk::SampleCountFlagBits samples, const vk::SampleMask* sampleMask)
    {
        vk::CommandBuffer(_internal).setSampleMaskEXT(samples, sampleMask);
    }

    inline void SetAlphaToCoverageEnable(vk::Bool32 enable) const
    {
        vk::CommandBuffer(_internal).setAlphaToCoverageEnableEXT(enable);
    }

    inline void SetAlphaToOneEnable(vk::Bool32 enable) const
    {
        vk::CommandBuffer(_internal).setAlphaToOneEnableEXT(enable);
    }

    inline void SetDepthClampEnable(vk::Bool32 enable) const
    {
        vk::CommandBuffer(_internal).setDepthClampEnableEXT(enable);
    }

    inline void SetVertexInput(std::span<const vk::VertexInputBindingDescription2EXT> vertexBindingDescriptions, std::span<const vk::VertexInputAttributeDescription2EXT> vertexAttributeDescriptions) const
    {
        vk::CommandBuffer(_internal).setVertexInputEXT(vertexBindingDescriptions, vertexAttributeDescriptions);
    }

    inline void BindShaders(std::span<const vk::ShaderStageFlagBits> stages, std::span<const vk::ShaderEXT> pShaders) const
    {
        vk::CommandBuffer(_internal).bindShadersEXT(stages, pShaders);
    }

    inline void Dispatch(uint32_t x, uint32_t y, uint32_t z) const
    {
        _internal.dispatch(x, y, z);
    }

    inline void BeginMarker(const char* name) const
    {
        vk::DebugUtilsLabelEXT label{};
        label.pLabelName = name;
        vk::CommandBuffer(_internal).beginDebugUtilsLabelEXT(label);
    }

    inline void EndMarker() const
    {
        vk::CommandBuffer(_internal).endDebugUtilsLabelEXT();
    }

    inline vk::CommandBufferSubmitInfo GetSubmitInfo() const
    {
        return vk::CommandBufferSubmitInfo{}
            .setCommandBuffer(_internal);
    }

    inline VkCommandBuffer GetInternal() const { return _internal; }

    private:

    void CreateSingleTime();
    void CreateMultiUsage();

    vk::CommandPool _pool;
    vk::Queue _queue = nullptr;
    vk::CommandBuffer _internal = nullptr;
};
