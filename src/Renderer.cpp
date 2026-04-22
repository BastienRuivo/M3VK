#include "header/Renderer.h"
#include "header/Material.h"
#include "header/Registries/MeshRegistry.h"
#include <vulkan/vulkan_core.h>

void Renderer::AddMesh(const SubMesh& mesh, const Material& material)
{
    _meshes.push_back(mesh);
    _materials.push_back(material);
}

void Renderer::Draw(const CommandBuffer& cmdBuffer, VkPipelineLayout layout) const
{
    for (int i = 0; i < _meshes.size(); i++)
    {
        const SubMesh& mesh = _meshes[i];
        const Material& material = _materials[i];

        cmdBuffer.BindDescriptorSets(layout, *material.DescriptorSet, 1);
        cmdBuffer.PushConstants(layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ObjectData), &_data);
        cmdBuffer.DrawIndexed(mesh.firstIndex, mesh.indexCount, mesh.vertexOffset);
    }
}

void Renderer::Update(float time, float dt)
{
    //_rotation = ProjectHelper::EulerToQuat(glm::vec3(0, dt * 100.0f, 0)) * _rotation;
    //_data.localToWorldMatrix = glm::translate(glm::mat4(1.0f), _position) * glm::toMat4(_rotation) * glm::scale(glm::mat4(1.0f), _scale);
}
