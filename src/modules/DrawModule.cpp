#include "modules/DrawModule.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

DrawModule::DrawModule(ShaderLibrary::VertexBinding vertexBinding, ShaderLibrary::FragmentBinding fragmentBinding)
    : VertexBinding(vertexBinding), FragmentBinding(fragmentBinding) {}

DrawModule::~DrawModule() {}

void DrawModule::Execute(const CommandBuffer& cmdBuffer, VkPipelineLayout layout, const GraphicsBuffer& indirectBuffer, uint32_t drawOffset, uint32_t drawCount, bool wireframe) const
{
    VertexBinding.Bind(cmdBuffer);
    FragmentBinding.Bind(cmdBuffer);

    if(wireframe)
    {
        cmdBuffer.SetPolygonMode(VK_POLYGON_MODE_LINE);
    }

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
