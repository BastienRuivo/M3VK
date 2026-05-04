#pragma once

#include "application/ApplicationInfo.h"
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
    glm::mat4 modelMatrix;
    uint32_t materialId;
    uint32_t pad0;
    uint32_t pad1;
    uint32_t pad2;
};

class MeshRegistry : public Registry
{
    public:
    MeshRegistry(size_t vertexBufferSize = ApplicationInfo::Constant::VertexBufferMaxSize, size_t indexBufferSize = ApplicationInfo::Constant::IndexBufferMaxSize, size_t indirectBufferSize = ApplicationInfo::Constant::DrawIndirectBufferMaxSize);

    uint32_t RegisterMesh(std::span<const Vertex> vertices, std::span<const uint32_t> indices);
    uint32_t RegisterInstance(InstanceData instance);
    VkDrawIndexedIndirectCommand& RegisterIndirectCommand(VkDrawIndexedIndirectCommand command);

    void UploadAndRelease(VkQueue queue, VkCommandPool cmdPool) override;
    void Bind(const CommandBuffer& cmdBuffer, VkPipelineLayout layout) const override;
    void Draw(const CommandBuffer& cmdBuffer, VkPipelineLayout layout) const;

    inline VkDescriptorBufferInfo InstanceBufferInfo() const { return _instanceDataBuffer.GetDescriptorBufferInfo(0, _instanceDataBuffer.GetCount()); }

    private:
    std::vector<Vertex> _cpuVertices;
    std::vector<uint32_t> _cpuIndices;
    std::vector<InstanceData> _cpuInstances;
    std::vector<VkDrawIndexedIndirectCommand> _cpuIndirectCommands;

    GeometryBuffer _vertexBuffer;
    GeometryBuffer _indexBuffer;
    GeometryBuffer _indirectBuffer;
    GeometryBuffer _instanceDataBuffer;
};
