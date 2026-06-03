#include "modules/DrawModule.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

DrawModule::DrawModule(ShaderLibrary::VertexBinding vertexBinding, ShaderLibrary::FragmentBinding fragmentBinding)
    : _vertexBinding(vertexBinding), _fragmentBinding(fragmentBinding) {}

DrawModule::~DrawModule() {}

void DrawModule::Execute(const CommandBuffer& cmdBuffer, VkPipelineLayout layout, const GraphicsBuffer& indirectBuffer, uint32_t drawCount, bool wireframe) const
{
    _vertexBinding.Bind(cmdBuffer);
    _fragmentBinding.Bind(cmdBuffer);

    if(wireframe)
    {
        cmdBuffer.SetPolygonMode(VK_POLYGON_MODE_LINE);
    }

    cmdBuffer.DrawIndexedIndirect(indirectBuffer.Internal(), 0, drawCount, indirectBuffer.GetStride());
}
