#pragma once

#include "asset/Material.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "rendering/Camera.h"
#include "rendering/CommandBuffer.h"
#include "registry/MeshRegistry.h"
#include <vector>

struct ObjectData
{
    alignas(16) glm::mat4 localToWorldMatrix;
};

class Renderer
{
    public:

    Renderer(std::vector<SubMesh> meshes, std::vector<Material> materials, glm::vec3 position, glm::quat rotation, glm::vec3 scale) : _meshes(meshes), _materials(materials), _position(position), _rotation(rotation), _scale(scale)
    {
        _data.localToWorldMatrix = glm::translate(glm::mat4(1.0f), _position) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), _scale);
    }
    Renderer(SubMesh mesh, Material material, glm::vec3 position, glm::quat rotation, glm::vec3 scale) : Renderer(std::vector<SubMesh>(1, mesh), std::vector<Material>(1, material), position, rotation, scale)
    {
    }
    Renderer(glm::vec3 position, glm::quat rotation, glm::vec3 scale) : Renderer(std::vector<SubMesh>(), std::vector<Material>(), position, rotation, scale)
    {
    }

    Renderer(const Renderer& other) = delete;
    Renderer& operator=(const Renderer& other) = delete;

    Renderer(Renderer&& other) noexcept
    {
        _meshes = std::move(other._meshes);
        _materials = std::move(other._materials);
        _position = other._position;
        _rotation = other._rotation;
        _scale = other._scale;
        _data = other._data;
    }

    Renderer& operator=(Renderer&& other) noexcept
    {
        if(this != &other)
        {
            _meshes = std::move(other._meshes);
            _materials = std::move(other._materials);
            _position = other._position;
            _rotation = other._rotation;
            _scale = other._scale;
            _data = other._data;
        }
        return *this;
    }

    void AddMesh(const SubMesh& mesh, const Material& material);
    void Draw(const CommandBuffer& cmdBuffer, VkPipelineLayout layout) const;
    //delta time
    void Update(float time, float dt);

    void SetPosition(glm::vec3 position)
    {
        _position = position;
        _data.localToWorldMatrix = glm::translate(glm::mat4(1.0f), _position) * glm::toMat4(_rotation) * glm::scale(glm::mat4(1.0f), _scale);
    }

    void SetRotation(glm::quat rotation)
    {
        _rotation = rotation;
        _data.localToWorldMatrix = glm::translate(glm::mat4(1.0f), _position) * glm::toMat4(_rotation) * glm::scale(glm::mat4(1.0f), _scale);
    }

    void SetScale(glm::vec3 scale)
    {
        _scale = scale;
        _data.localToWorldMatrix = glm::translate(glm::mat4(1.0f), _position) * glm::toMat4(_rotation) * glm::scale(glm::mat4(1.0f), _scale);
    }

    private:
    std::vector<SubMesh> _meshes;
    std::vector<Material> _materials;

    glm::vec3 _position;
    glm::quat _rotation;
    glm::vec3 _scale;

    ObjectData _data;
};
