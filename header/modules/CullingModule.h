#pragma once

#include "modules/Module.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

class CullingModule : public Module
{
public:

    struct CullingConstants
    {
        uint32_t InstanceCount;
        uint32_t DrawCount;
        uint32_t HizIndex;
    };

    CullingModule(ShaderLibrary& shaderLibrary, BindingManager& descriptorAllocator);
    ~CullingModule();

    CullingModule(CullingModule&& other) noexcept;
    CullingModule& operator=(CullingModule&& other) noexcept;

    CullingModule(const CullingModule&) = delete;
    CullingModule& operator=(const CullingModule&) = delete;

    void DoUI(const UserInterface& ui) override;
    void Execute(const CommandBuffer& cmdBuffer, uint32_t hizIndex, const GraphicsBuffer& cameraBuffer, const GeometryBuffer& indirectBuffer, const GeometryBuffer& instanceBuffer, VkPipelineLayout layout) const;
    void Barrier(const CommandBuffer& cmdBuffer, uint32_t instanceCount, uint32_t drawCount, VkAccessFlagBits2 src, VkPipelineStageFlagBits2 srcStage, VkAccessFlagBits2 dst, VkPipelineStageFlagBits2 dstStage) const;
    inline const GraphicsBuffer& VisibleInstanceIndirectionBuffer() const { return _visibleIndirectionBuffer; }
    inline const GraphicsBuffer& VisibleIndirectBuffer() const { return _visibleIndirectBuffer; }

private:
    GraphicsBuffer _visibleIndirectionBuffer;
    GraphicsBuffer _visibleIndirectBuffer;
    ShaderLibrary::ComputeKernel _cullingKernel;
    ShaderLibrary::ComputeKernel _cullingInitKernel;
};
