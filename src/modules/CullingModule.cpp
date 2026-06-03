#include "modules/CullingModule.h"
#include "rendering/BufferHelper.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

#include "ShaderBindings.h"
#include "Instancing.h"

CullingModule::CullingModule(ShaderLibrary& shaderLibrary, DescriptorAllocator& allocator)
: _visibleIndirectBuffer(allocator, BINDING_VISIBLE_DRAW_INDIRECT_BUFFER, GraphicsBuffer::BufferType::INDIRECT_DRAW, RessourceUsage::PerFrame, ApplicationInfo::Constant::DrawIndirectBufferMaxSize, sizeof(DrawIndexedIndirectPadded)),
_visibleIndirectionBuffer(allocator, BINDING_VISIBLE_INSTANCE_INDIRECTION_BUFFER, GraphicsBuffer::BufferType::STORAGE, RessourceUsage::PerFrame, ApplicationInfo::Constant::DrawIndirectBufferMaxSize, sizeof(uint32_t) * 4) // 16 bytes because SSBO have a 16 bytes alignment on Nvidia :/
{
    uint32_t cullingShader = shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "Culling.comp.spv", ShaderLibrary::Compute, allocator);

    auto& shaderHandler = shaderLibrary.Get(cullingShader);
    _cullingKernel =
    {
        .Shader = shaderHandler.Internal(),
        .GX = shaderHandler.Info().Compute.X,
        .GY = shaderHandler.Info().Compute.Y,
        .GZ = shaderHandler.Info().Compute.Z
    };

    uint32_t cullingInitShader = shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "CullingInit.comp.spv", ShaderLibrary::Compute, allocator);

    auto& cullingInitShaderHandler = shaderLibrary.Get(cullingInitShader);
    _cullingInitKernel =
    {
        .Shader = cullingInitShaderHandler.Internal(),
        .GX = cullingInitShaderHandler.Info().Compute.X,
        .GY = cullingInitShaderHandler.Info().Compute.Y,
        .GZ = cullingInitShaderHandler.Info().Compute.Z
    };
}

CullingModule::~CullingModule()
{
}

void CullingModule::Execute(const CommandBuffer& cmdBuffer, const GraphicsBuffer& cameraBuffer, const GeometryBuffer& indirectBuffer, const GeometryBuffer& instanceBuffer, VkPipelineLayout layout) const
{
    cmdBuffer.BeginMarker("Culling Module");
    {
        BufferIndexes indexes
        {
            .Cameras = cameraBuffer.GetGPUIndex(),
            .VisibleInstanceIndirections = _visibleIndirectionBuffer.GetGPUIndex(),
            .VisibleDrawIndirects = _visibleIndirectBuffer.GetGPUIndex(),
        };

        CullingConstants constants
        {
            .InstanceCount = instanceBuffer.GetCurrentIndex(),
            .DrawCount = indirectBuffer.GetCurrentIndex()
        };

        cmdBuffer.PushConstants(layout, 0, sizeof(BufferIndexes), &indexes);
        cmdBuffer.PushConstants(layout, sizeof(BufferIndexes), sizeof(CullingConstants), &constants);

        Barrier(cmdBuffer, constants.InstanceCount, constants.DrawCount, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT);

        cmdBuffer.BeginMarker("Culling Init");
        {
            _cullingInitKernel.Bind(cmdBuffer);
            _cullingInitKernel.CeilDispatch(cmdBuffer, constants.DrawCount);
        }
        cmdBuffer.EndMarker();

        cmdBuffer.BeginMarker("Culling Pass");
        {
            _cullingKernel.Bind(cmdBuffer);
            _cullingKernel.CeilDispatch(cmdBuffer, constants.InstanceCount);
        }
        cmdBuffer.EndMarker();

        Barrier(cmdBuffer, constants.InstanceCount, constants.DrawCount, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
    }
    cmdBuffer.EndMarker();
}

void CullingModule::Barrier(const CommandBuffer& cmdBuffer, uint32_t instanceCount, uint32_t drawCount, VkAccessFlagBits src, VkAccessFlagBits dst) const
{
    std::array<VkBufferMemoryBarrier, 2> bufferBarrier =
    {
        BufferHelper::BufferBarrier(_visibleIndirectBuffer, 0, drawCount, src, dst),
        BufferHelper::BufferBarrier(_visibleIndirectionBuffer, 0, instanceCount, src, dst)
    };

    cmdBuffer.Barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, nullptr, 0, bufferBarrier.data(), bufferBarrier.size(), nullptr, 0);
}
