#include "modules/DrawModule.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include <cstdint>
#include <vulkan/vulkan.hpp>

DrawModule::DrawModule(ShaderLibrary::VertexBinding vertexBinding, ShaderLibrary::FragmentBinding fragmentBinding)
    : VertexBinding(vertexBinding), FragmentBinding(fragmentBinding) {}

DrawModule::~DrawModule() {}

void DrawModule::Execute(const CommandBuffer& cmdBuffer, vk::PipelineLayout layout, const GraphicsBuffer& indirectBuffer, uint32_t drawOffset, uint32_t drawCount) const
{
    VertexBinding.Bind(cmdBuffer);
    FragmentBinding.Bind(cmdBuffer);

    cmdBuffer.DrawIndexedIndirect(indirectBuffer.Internal(), drawOffset, drawCount, indirectBuffer.GetStride());
}

void DrawModule::Execute(const CommandBuffer& cmdBuffer, vk::PipelineLayout layout, const VertexState& vertexState, const FragmentState& fragState, const GraphicsBuffer& indirectBuffer, uint32_t drawOffset, uint32_t drawCount) const
{
    const vk::ShaderStageFlagBits vertexStage = vk::ShaderStageFlagBits::eVertex;
    cmdBuffer.BindShaders({&vertexStage, 1}, {&VertexBinding.Handle, 1});
    vertexState.Bind(cmdBuffer);

    const vk::ShaderStageFlagBits fragStage = vk::ShaderStageFlagBits::eFragment;
    cmdBuffer.BindShaders({&fragStage, 1}, {&FragmentBinding.Handle, 1});
    fragState.Bind(cmdBuffer);

    cmdBuffer.DrawIndexedIndirect(indirectBuffer.Internal(), drawOffset, drawCount, indirectBuffer.GetStride());
}

 DrawModule::DrawModule(DrawModule&& other) noexcept
 {
    VertexBinding = std::move(other.VertexBinding);
    FragmentBinding = std::move(other.FragmentBinding);
 }

DrawModule& DrawModule::operator=(DrawModule&& other) noexcept
{
    if(this != &other)
    {
        VertexBinding = std::move(other.VertexBinding);
        FragmentBinding = std::move(other.FragmentBinding);
    }

    return *this;
}
