#pragma once

#include "Material.h"
#include "application/ApplicationInfo.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GraphicsBuffer.h"
#include "allocation/Registry.h"
#include "asset/Vertex.h"
#include <cstdint>
#include <span>
#include <vector>

#include "Instancing.h"

struct MeshHandle
{
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t firstVertex;
};

class MeshRegistry : public Registry
{
    public:
    struct LayerMaterialInfo
    {
        uint32_t offset;
        uint32_t count;
    };

    MeshRegistry(DescriptorAllocator& allocator, size_t vertexBufferSize = ApplicationInfo::Constant::VertexBufferMaxSize, size_t indexBufferSize = ApplicationInfo::Constant::IndexBufferMaxSize, size_t indirectBufferSize = ApplicationInfo::Constant::DrawIndirectBufferMaxSize);

    uint32_t RegisterMesh(MaterialType type, std::span<const Vertex> vertices, std::span<const uint32_t> indices);
    uint32_t RegisterInstance(MaterialType type, InstanceData instance);

    void UploadAndRelease(VkQueue queue, VkCommandPool cmdPool) override;
    void Bind(const CommandBuffer& cmdBuffer, VkPipelineLayout layout) const override;

    inline VkDescriptorBufferInfo InstanceBufferInfo() const { return _instanceDataBuffer.GetDescriptorBufferInfo(0, _instanceDataBuffer.GetCount()); }
    inline VkDescriptorBufferInfo IndirectBufferInfo() const { return _indirectBuffer.GetDescriptorBufferInfo(0, _indirectBuffer.GetCount()); }

    inline const GeometryBuffer& VertexBuffer() const { return _vertexBuffer; }
    inline const GeometryBuffer& IndexBuffer() const { return _indexBuffer; }
    inline const GeometryBuffer& IndirectBuffer() const { return _indirectBuffer; }
    inline const GeometryBuffer& InstanceDataBuffer() const { return _instanceDataBuffer; }
    inline const LayerMaterialInfo& GetIndirectDrawInfo(MaterialType type) const { return _indirectDrawInfoPerMaterial[static_cast<uint32_t>(type)]; }
    inline InstanceData& LastInstance(MaterialType type) { return _uploadInstances[type].back(); }

    private:
    std::vector<Vertex> _uploadVertices;
    std::vector<uint32_t> _uploadIndices;
    std::array<std::vector<InstanceData>, MaterialType::Count> _uploadInstances;
    std::array<std::vector<DrawIndexedIndirectPadded>, MaterialType::Count> _uploadIndirectCommands;
    std::array<LayerMaterialInfo, MaterialType::Count> _indirectDrawInfoPerMaterial;
    std::array<LayerMaterialInfo, MaterialType::Count> _indirectInstanceInfoPerMaterial;

    GeometryBuffer _vertexBuffer;
    GeometryBuffer _indexBuffer;
    GeometryBuffer _indirectBuffer;
    GeometryBuffer _instanceDataBuffer;
};
