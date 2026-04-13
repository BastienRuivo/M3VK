#pragma once

#include "header/CommandBuffer.h"
#include "header/MeshRegistry.h"
#include <initializer_list>
#include <vector>

struct ObjectData
{
    alignas(16) glm::mat4 localToWorldMatrix;
};

class Renderer
{
    public:

    Renderer(std::initializer_list<SubMesh> meshes, ObjectData data) : _meshes(meshes), _data(data) {}
    Renderer(SubMesh mesh, ObjectData data) : _meshes({mesh}), _data(data) {}

    void AddMesh(const SubMesh& mesh);
    void Draw(const CommandBuffer& cmdBuffer, VkPipelineLayout layout) const;

    private:
    std::vector<SubMesh> _meshes;
    ObjectData _data;
};
