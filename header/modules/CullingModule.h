#pragma once

#include "modules/Module.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include <cstdint>
#include <vulkan/vulkan.hpp>

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

    CullingModule(CullingModule&& other) noexcept = default;
    CullingModule& operator=(CullingModule&& other) noexcept = default;

    CullingModule(const CullingModule&) = delete;
    CullingModule& operator=(const CullingModule&) = delete;

    void DoUI(const UserInterface& ui) override;
    void Execute(const CommandBuffer& cmdBuffer, uint32_t hizIndex, const GraphicsBuffer& cameraBuffer, const GeometryBuffer& indirectBuffer, const GeometryBuffer& instanceBuffer, vk::PipelineLayout layout) const;
    void Barrier(const CommandBuffer& cmdBuffer, uint32_t instanceCount, uint32_t drawCount, vk::AccessFlags2 src, vk::PipelineStageFlags2 srcStage, vk::AccessFlags2 dst, vk::PipelineStageFlags2 dstStage) const;
    inline const GraphicsBuffer& VisibleInstanceIndirectionBuffer() const { return _visibleIndirectionBuffer; }
    inline const GraphicsBuffer& VisibleIndirectBuffer() const { return _visibleIndirectBuffer; }

private:
    GraphicsBuffer _visibleIndirectionBuffer;
    GraphicsBuffer _visibleIndirectBuffer;
    ShaderLibrary::ComputeKernel _cullingKernel;
    ShaderLibrary::ComputeKernel _cullingInitKernel;
};
