#pragma once

#include "application/ApplicationInfo.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GraphicsBuffer.h"
#include "registry/Registry.h"
#include "asset/Vertex.h"
#include <cstdint>
#include <span>
#include <vector>
struct SubMesh
{
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t vertexOffset;
};

class MeshRegistry : public Registry
{
    public:
    MeshRegistry(size_t vertexBufferSize = ApplicationInfo::Constant::VertexBufferMaxSize, size_t indexBufferSize = ApplicationInfo::Constant::IndexBufferMaxSize);

    SubMesh Register(std::span<const Vertex> vertices, std::span<const uint32_t> indices);

    void UploadAndRelease(VkQueue queue, VkCommandPool cmdPool) override;
    void Bind(const CommandBuffer& cmdBuffer) const override;

    private:
    std::vector<Vertex> _cpuVertices;
    std::vector<uint32_t> _cpuIndices;

    GeometryBuffer _vertexBuffer;
    GeometryBuffer _indexBuffer;
};
