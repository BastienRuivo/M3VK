#include "modules/CullingModule.h"
#include "rendering/BufferHelper.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include <cstdint>
#include <vulkan/vulkan.hpp>

#include "ShaderBindings.h"
#include "Instancing.h"

CullingModule::CullingModule(ShaderLibrary& shaderLibrary, BindingManager& allocator)
: _visibleIndirectBuffer(allocator, BINDING_VISIBLE_DRAW_INDIRECT_BUFFER, GraphicsBuffer::BufferType::INDIRECT_DRAW, RessourceUsage::PerFrame, ApplicationInfo::Constant::DrawIndirectBufferMaxSize, sizeof(DrawIndexedIndirectPadded)),
_visibleIndirectionBuffer(allocator, BINDING_VISIBLE_INSTANCE_INDIRECTION_BUFFER, GraphicsBuffer::BufferType::STORAGE, RessourceUsage::PerFrame, ApplicationInfo::Constant::DrawIndirectBufferMaxSize, sizeof(uint32_t) * 4) // 16 bytes because SSBO have a 16 bytes alignment on Nvidia :/
{
    _cullingKernel = shaderLibrary.RegisterComputeKernel(std::filesystem::path(SHADER_DIRECTORY) / "Culling.comp.spv", allocator);
    _cullingInitKernel = shaderLibrary.RegisterComputeKernel(std::filesystem::path(SHADER_DIRECTORY) / "CullingInit.comp.spv", allocator);
}

CullingModule::~CullingModule()
{
}

void CullingModule::Execute(const CommandBuffer& cmdBuffer, uint32_t hizIndex, const GraphicsBuffer& cameraBuffer, const GeometryBuffer& indirectBuffer, const GeometryBuffer& instanceBuffer, vk::PipelineLayout layout) const
{
    cmdBuffer.BeginMarker("Culling Module");
    {
        CullingConstants constants
        {
            .InstanceCount = instanceBuffer.GetCurrentIndex(),
            .DrawCount = indirectBuffer.GetCurrentIndex(),
            .HizIndex = hizIndex
        };

        cmdBuffer.PushConstants(layout, COMMON_INDEXES_OFFSET, sizeof(CullingConstants), &constants);

        Barrier(cmdBuffer, constants.InstanceCount, constants.DrawCount, vk::AccessFlagBits2::eIndirectCommandRead, vk::PipelineStageFlagBits2::eDrawIndirect, vk::AccessFlagBits2::eShaderWrite, vk::PipelineStageFlagBits2::eComputeShader);

        cmdBuffer.BeginMarker("Culling Init");
        {
            _cullingInitKernel.Bind(cmdBuffer);
            _cullingInitKernel.CeilDispatch(cmdBuffer, constants.DrawCount);
        }
        cmdBuffer.EndMarker();

        Barrier(cmdBuffer, constants.InstanceCount, constants.DrawCount, vk::AccessFlagBits2::eShaderWrite, vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite, vk::PipelineStageFlagBits2::eComputeShader);

        cmdBuffer.BeginMarker("Culling Pass");
        {
            _cullingKernel.Bind(cmdBuffer);
            _cullingKernel.CeilDispatch(cmdBuffer, constants.InstanceCount);
        }
        cmdBuffer.EndMarker();

        Barrier(cmdBuffer, constants.InstanceCount, constants.DrawCount, vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite, vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eIndirectCommandRead, vk::PipelineStageFlagBits2::eDrawIndirect);
    }
    cmdBuffer.EndMarker();
}

void CullingModule::Barrier(const CommandBuffer& cmdBuffer, uint32_t instanceCount, uint32_t drawCount, vk::AccessFlags2 src, vk::PipelineStageFlags2 srcStage, vk::AccessFlags2 dst, vk::PipelineStageFlags2 dstStage) const
{
    std::array<vk::BufferMemoryBarrier2, 2> bufferBarrier =
    {
        BufferHelper::BufferBarrier(_visibleIndirectBuffer, 0, drawCount, src, srcStage, dst, dstStage),
        BufferHelper::BufferBarrier(_visibleIndirectionBuffer, 0, instanceCount, src, srcStage, dst, dstStage)
    };

    cmdBuffer.Barrier(bufferBarrier);
}

void CullingModule::DoUI(const UserInterface& ui)
{
    ImGui::Begin("CullingModule");
    {

    }
    ImGui::End();
}
