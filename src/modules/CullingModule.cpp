#include "modules/CullingModule.h"
#include "application/ApplicationInfo.h"
#include "registry/MeshRegistry.h"
#include "rendering/BufferHelper.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

CullingModule::CullingModule(ShaderLibrary& shaderLibrary, DescriptorAllocator& allocator)
: _visibleIndirectBuffer(allocator, BINDING_VISIBLE_DRAW_INDIRECT_BUFFER, GraphicsBuffer::BufferType::INDIRECT_DRAW, RessourceUsage::PerFrame, ApplicationInfo::Constant::DrawIndirectBufferMaxSize, sizeof(DrawIndexedIndirectPadded)),
_visibleInstanceBuffer(allocator, BINDING_VISIBLE_INSTANCE_DATA_BUFFER, GraphicsBuffer::BufferType::STORAGE, RessourceUsage::PerFrame, ApplicationInfo::Constant::DrawIndirectBufferMaxSize, sizeof(InstanceData))
{
    uint32_t shader = shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "Culling.comp.spv", ShaderLibrary::Compute, allocator);

    auto& shaderHandler = shaderLibrary.Get(shader);
    _cullingKernel =
    {
        .Shader = shaderHandler.Internal(),
        .GX = shaderHandler.Info().Compute.X,
        .GY = shaderHandler.Info().Compute.Y,
        .GZ = shaderHandler.Info().Compute.Z
    };
}

CullingModule::~CullingModule()
{
}

void CullingModule::Execute(const CommandBuffer& cmdBuffer, const GraphicsBuffer& cameraBuffer, const GeometryBuffer& indirectBuffer, const GeometryBuffer& instanceBuffer, VkPipelineLayout layout, uint32_t instanceCount) const
{
    _cullingKernel.Bind(cmdBuffer);
    Barrier(cmdBuffer, instanceCount, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT);

    BufferIndexes indexes
    {
        .Cameras = cameraBuffer.GetGPUIndex(),
        .VisibleInstanceDatas = _visibleInstanceBuffer.GetGPUIndex(),
        .VisibleDrawIndirects = _visibleIndirectBuffer.GetGPUIndex(),
    };

    CullingConstants constants
    {
        .InstanceCount = instanceCount
    };

    cmdBuffer.PushConstants(layout, 0, sizeof(BufferIndexes), &indexes);
    cmdBuffer.PushConstants(layout, sizeof(BufferIndexes), sizeof(CullingConstants), &constants);

    _cullingKernel.CeilDispatch(cmdBuffer, instanceCount);

    Barrier(cmdBuffer, instanceCount, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
}

void CullingModule::Barrier(const CommandBuffer& cmdBuffer, uint32_t instanceCount, VkAccessFlagBits src, VkAccessFlagBits dst) const
{
    std::array<VkBufferMemoryBarrier, 2> bufferBarrier =
    {
        BufferHelper::BufferBarrier(_visibleIndirectBuffer, 0, instanceCount, src, dst),
        BufferHelper::BufferBarrier(_visibleInstanceBuffer, 0, instanceCount, src, dst)
    };

    cmdBuffer.Barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, nullptr, 0, bufferBarrier.data(), bufferBarrier.size(), nullptr, 0);
}
