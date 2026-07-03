#include "modules/CullingModule.h"
#include "rendering/BufferHelper.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

#include "ShaderBindings.h"
#include "Instancing.h"

CullingModule::CullingModule(ShaderLibrary& shaderLibrary, BindingManager& allocator)
: _visibleIndirectBuffer(allocator, BINDING_VISIBLE_DRAW_INDIRECT_BUFFER, GraphicsBuffer::BufferType::INDIRECT_DRAW, RessourceUsage::PerFrame, ApplicationInfo::Constant::DrawIndirectBufferMaxSize, sizeof(DrawIndexedIndirectPadded)),
_visibleIndirectionBuffer(allocator, BINDING_VISIBLE_INSTANCE_INDIRECTION_BUFFER, GraphicsBuffer::BufferType::STORAGE, RessourceUsage::PerFrame, ApplicationInfo::Constant::DrawIndirectBufferMaxSize, sizeof(uint32_t) * 4) // 16 bytes because SSBO have a 16 bytes alignment on Nvidia :/
{
    uint32_t cullingShader = shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "Culling.comp.spv", ShaderLibrary::Compute, allocator, {});

    auto& cullingShaderInfo = shaderLibrary.Get(cullingShader);
    _cullingKernel =
    {
        .LibraryIndex = cullingShader,
        .Handle = cullingShaderInfo.Handle,
        .GX = cullingShaderInfo.Info.Compute.X,
        .GY = cullingShaderInfo.Info.Compute.Y,
        .GZ = cullingShaderInfo.Info.Compute.Z
    };

    uint32_t cullingInitShader = shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "CullingInit.comp.spv", ShaderLibrary::Compute, allocator, {});

    auto& initShaderInfo = shaderLibrary.Get(cullingInitShader);
    _cullingInitKernel =
    {
        .LibraryIndex = cullingInitShader,
        .Handle = initShaderInfo.Handle,
        .GX = initShaderInfo.Info.Compute.X,
        .GY = initShaderInfo.Info.Compute.Y,
        .GZ = initShaderInfo.Info.Compute.Z
    };
}

CullingModule::~CullingModule()
{
}

void CullingModule::Execute(const CommandBuffer& cmdBuffer, const GraphicsBuffer& cameraBuffer, const GeometryBuffer& indirectBuffer, const GeometryBuffer& instanceBuffer, VkPipelineLayout layout) const
{
    cmdBuffer.BeginMarker("Culling Module");
    {
        CommonIndexes indexes
        {
            .DebugIndex = 0,
            .Cameras = cameraBuffer.GetGPUIndex(),
            .VisibleInstanceIndirections = _visibleIndirectionBuffer.GetGPUIndex(),
            .VisibleDrawIndirects = _visibleIndirectBuffer.GetGPUIndex(),
        };

        CullingConstants constants
        {
            .InstanceCount = instanceBuffer.GetCurrentIndex(),
            .DrawCount = indirectBuffer.GetCurrentIndex()
        };

        cmdBuffer.PushConstants(layout, 0, sizeof(CommonIndexes), &indexes);
        cmdBuffer.PushConstants(layout, sizeof(CommonIndexes), sizeof(CullingConstants), &constants);

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

void CullingModule::Barrier(const CommandBuffer& cmdBuffer, uint32_t instanceCount, uint32_t drawCount, VkAccessFlagBits2 src, VkAccessFlagBits2 dst) const
{
    std::array<VkBufferMemoryBarrier2, 2> bufferBarrier =
    {
        BufferHelper::BufferBarrier(_visibleIndirectBuffer, 0, drawCount, src, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, dst, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT),
        BufferHelper::BufferBarrier(_visibleIndirectionBuffer, 0, instanceCount, src, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, dst, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT)
    };

    cmdBuffer.Barrier(bufferBarrier);
}
