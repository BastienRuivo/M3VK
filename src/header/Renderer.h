#pragma once

#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "header/Camera.h"
#include "header/CommandBuffer.h"
#include "header/MeshRegistry.h"
#include <cmath>
#include <initializer_list>
#include <vector>

struct ObjectData
{
    alignas(16) glm::mat4 localToWorldMatrix;
};

class Renderer
{
    public:

    Renderer(std::initializer_list<SubMesh> meshes, glm::vec3 position, glm::quat rotation, glm::vec3 scale) : _meshes(meshes), _position(position), _rotation(rotation), _scale(scale)
    {
        _data.localToWorldMatrix = glm::translate(glm::mat4(1.0f), _position) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), _scale);
    }
    Renderer(SubMesh mesh, glm::vec3 position, glm::quat rotation, glm::vec3 scale) : Renderer({mesh}, position, rotation, scale) {}

    void AddMesh(const SubMesh& mesh);
    void Draw(const CommandBuffer& cmdBuffer, VkPipelineLayout layout) const;
    //delta time
    void Update(float time, float dt);

    private:
    std::vector<SubMesh> _meshes;
    glm::vec3 _position;
    glm::quat _rotation;
    glm::vec3 _scale;
    ObjectData _data;
};
