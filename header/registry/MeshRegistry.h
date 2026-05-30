#pragma once

#include "application/ApplicationInfo.h"
#include "glm/ext/vector_float3.hpp"
#include "rendering/CommandBuffer.h"
#include "rendering/GraphicsBuffer.h"
#include "registry/Registry.h"
#include "asset/Vertex.h"
#include <cstdint>
#include <span>
#include <vector>

struct MeshHandle
{
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t firstVertex;
};

struct InstanceData
{
    alignas(16) glm::mat4 modelMatrix;

    glm::vec3 AABBMin;
    uint32_t materialId;

    glm::vec3 AABBMax;
    uint32_t meshId;
};

struct DrawIndexedIndirectPadded {
    uint32_t    indexCount;
    uint32_t    instanceCount;
    uint32_t    firstIndex;
    int32_t     vertexOffset;
    uint32_t    firstInstance;
    uint32_t pad0[3];
};

class MeshRegistry : public Registry
{
    public:
    MeshRegistry(DescriptorAllocator& allocator, size_t vertexBufferSize = ApplicationInfo::Constant::VertexBufferMaxSize, size_t indexBufferSize = ApplicationInfo::Constant::IndexBufferMaxSize, size_t indirectBufferSize = ApplicationInfo::Constant::DrawIndirectBufferMaxSize);

    uint32_t RegisterMesh(std::span<const Vertex> vertices, std::span<const uint32_t> indices);
    uint32_t RegisterInstance(InstanceData instance);
    DrawIndexedIndirectPadded& RegisterIndirectCommand(DrawIndexedIndirectPadded command);

    void UploadAndRelease(VkQueue queue, VkCommandPool cmdPool) override;
    void Bind(const CommandBuffer& cmdBuffer, VkPipelineLayout layout) const override;

    inline VkDescriptorBufferInfo InstanceBufferInfo() const { return _instanceDataBuffer.GetDescriptorBufferInfo(0, _instanceDataBuffer.GetCount()); }
    inline VkDescriptorBufferInfo IndirectBufferInfo() const { return _indirectBuffer.GetDescriptorBufferInfo(0, _indirectBuffer.GetCount()); }

    inline const GeometryBuffer& VertexBuffer() const { return _vertexBuffer; }
    inline const GeometryBuffer& IndexBuffer() const { return _indexBuffer; }
    inline const GeometryBuffer& IndirectBuffer() const { return _indirectBuffer; }
    inline const GeometryBuffer& InstanceDataBuffer() const { return _instanceDataBuffer; }

    private:
    std::vector<Vertex> _cpuVertices;
    std::vector<uint32_t> _cpuIndices;
    std::vector<InstanceData> _cpuInstances;
    std::vector<DrawIndexedIndirectPadded> _cpuIndirectCommands;

    GeometryBuffer _vertexBuffer;
    GeometryBuffer _indexBuffer;
    GeometryBuffer _indirectBuffer;
    GeometryBuffer _instanceDataBuffer;
};
