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
        uint32_t DrawCount;
    };

    CullingModule(ShaderLibrary& shaderLibrary, BindingManager& descriptorAllocator);
    ~CullingModule();

    void Execute(const CommandBuffer& cmdBuffer, const GraphicsBuffer& cameraBuffer, const GeometryBuffer& indirectBuffer, const GeometryBuffer& instanceBuffer, VkPipelineLayout layout) const;
    void Barrier(const CommandBuffer& cmdBuffer, uint32_t instanceCount, uint32_t drawCount, VkAccessFlagBits2 src, VkAccessFlagBits2 dst) const;
    inline const GraphicsBuffer& VisibleInstanceIndirectionBuffer() const { return _visibleIndirectionBuffer; }
    inline const GraphicsBuffer& VisibleIndirectBuffer() const { return _visibleIndirectBuffer; }

private:
    GraphicsBuffer _visibleIndirectionBuffer;
    GraphicsBuffer _visibleIndirectBuffer;
    ShaderLibrary::ComputeKernel _cullingKernel;
    ShaderLibrary::ComputeKernel _cullingInitKernel;
};
