#include "header/Renderer.h"
#include "glm/ext/vector_float3.hpp"
#include "header/ProjectHelper.h"

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
    _rotation = ProjectHelper::EulerToQuat(glm::vec3(0, dt * 100.0f, 0)) * _rotation;
    _data.localToWorldMatrix = glm::translate(glm::mat4(1.0f), _position) * glm::toMat4(_rotation) * glm::scale(glm::mat4(1.0f), _scale);
}
