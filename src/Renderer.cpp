#include "header/Renderer.h"

void Renderer::AddMesh(const SubMesh& mesh)
{
    _meshes.push_back(mesh);
}

void Renderer::Draw(const CommandBuffer& cmdBuffer, VkPipelineLayout layout) const
{
    for (const auto& mesh : _meshes)
    {
        cmdBuffer.PushConstants(layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ObjectData), &_data);
        cmdBuffer.DrawIndexed(mesh.firstIndex, mesh.indexCount, mesh.vertexOffset);
    }
}
