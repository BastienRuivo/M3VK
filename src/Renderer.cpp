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

void Renderer::Update(float time, float dt)
{
    _rotation = _rotation * glm::angleAxis(2.0f * dt, glm::vec3(0.0f, 1.0f, 0.0f));
    _data.localToWorldMatrix = glm::translate(glm::mat4(1.0f), _position) * glm::toMat4(_rotation) * glm::scale(glm::mat4(1.0f), _scale);
}
