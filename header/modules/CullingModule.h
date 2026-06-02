#pragma once

#include "rendering/CommandBuffer.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

class CullingModule
{
public:

    struct CullingConstants
    {
        uint32_t InstanceCount;
    };

    CullingModule(ShaderLibrary& shaderLibrary, DescriptorAllocator& descriptorAllocator);
    ~CullingModule();

    void Execute(const CommandBuffer& cmdBuffer, const GraphicsBuffer& cameraBuffer, const GeometryBuffer& indirectBuffer, const GeometryBuffer& instanceBuffer, VkPipelineLayout layout, uint32_t instanceCount) const;
    void Barrier(const CommandBuffer& cmdBuffer, uint32_t instanceCount, VkAccessFlagBits src, VkAccessFlagBits dst) const;
    inline const GraphicsBuffer& VisibleInstanceBuffer() const { return _visibleInstanceBuffer; }
    inline const GraphicsBuffer& VisibleIndirectBuffer() const { return _visibleIndirectBuffer; }

private:
    GraphicsBuffer _visibleInstanceBuffer;
    GraphicsBuffer _visibleIndirectBuffer;
    ShaderLibrary::ComputeKernel _cullingKernel;
};
