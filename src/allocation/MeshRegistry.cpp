#include "allocation/MeshRegistry.h"
#include "Material.h"
#include "application/ApplicationInfo.h"
#include <cassert>
#include <cstdint>
#include <vulkan/vulkan.hpp>

#include "ShaderBindings.h"

MeshRegistry::MeshRegistry(BindingManager& allocator, size_t vertexBufferSize, size_t indexBufferSize, size_t indirectBufferSize)
: _vertexBuffer(GraphicsBuffer::BufferType::VERTEX, RessourceUsage::Static, vertexBufferSize, sizeof(Vertex)),
    _indexBuffer(GraphicsBuffer::BufferType::INDEX, RessourceUsage::Static, indexBufferSize, sizeof(uint32_t)),
    _indirectBuffer(allocator, BINDING_CLEAR_DRAW_INDIRECT_BUFFER, GraphicsBuffer::BufferType::STORAGE, RessourceUsage::Static, indirectBufferSize, sizeof(DrawIndexedIndirectPadded)),
    _instanceDataBuffer(allocator, BINDING_INSTANCE_DATA_BUFFER, GraphicsBuffer::BufferType::STORAGE, RessourceUsage::Static, indirectBufferSize, sizeof(InstanceData))
{
}

uint32_t MeshRegistry::RegisterMesh(MaterialType type, std::span<const Vertex> vertices, std::span<const uint32_t> indices)
{
    MeshHandle subMesh
    {
        .firstIndex = static_cast<uint32_t>(_uploadIndices.size()),
        .indexCount = static_cast<uint32_t>(indices.size()),
        .firstVertex = static_cast<uint32_t>(_uploadVertices.size())
    };

    _uploadVertices.insert(_uploadVertices.end(), vertices.begin(), vertices.end());
    _uploadIndices.insert(_uploadIndices.end(), indices.begin(), indices.end());

    _uploadIndirectCommands[type].push_back(DrawIndexedIndirectPadded
    {
        .indexCount = subMesh.indexCount,
        .instanceCount = 0,
        .firstIndex = subMesh.firstIndex,
        .vertexOffset = static_cast<int32_t>(subMesh.firstVertex),
        .firstInstance = static_cast<uint32_t>(_uploadInstances[type].size())
    });

    return static_cast<uint32_t>(_uploadIndirectCommands[type].size() - 1);
}

uint32_t MeshRegistry::RegisterInstance(MaterialType type, InstanceData instance)
{
    _uploadInstances[type].push_back(instance);
    return static_cast<uint32_t>(_uploadInstances[type].size() - 1);
}

void MeshRegistry::UploadAndRelease(vk::Queue queue, vk::CommandPool cmdPool)
{
    assert(_uploadVertices.size() <= ApplicationInfo::Constant::VertexBufferMaxSize);
    assert(_uploadIndices.size() <= ApplicationInfo::Constant::IndexBufferMaxSize);
    assert(_uploadInstances.size() <= ApplicationInfo::Constant::DrawIndirectBufferMaxSize);

    if(_uploadIndices.size() == 0 || _uploadVertices.size() == 0 || _uploadInstances.size() == 0)
    {
        DebugLayer::Log(DebugLayer::LogType::WARNING, "No vertices or indices or instances to upload");
        return;
    }

    _vertexBuffer.CopyToBuffer(queue, cmdPool, _uploadVertices.data(), _uploadVertices.size() * _vertexBuffer.GetStride());
    _indexBuffer.CopyToBuffer(queue, cmdPool, _uploadIndices.data(), _uploadIndices.size() * _indexBuffer.GetStride());

    uint32_t offsetDraw = 0;
    uint32_t offsetInstance = 0;
    for(uint32_t i = 0; i < MaterialType::Count; i++)
    {
        _indirectDrawInfoPerMaterial[i].offset = offsetDraw;
        _indirectDrawInfoPerMaterial[i].count = static_cast<uint32_t>(_uploadIndirectCommands[i].size());
        offsetDraw += _indirectDrawInfoPerMaterial[i].count;

        _indirectInstanceInfoPerMaterial[i].offset = offsetInstance;
        _indirectInstanceInfoPerMaterial[i].count = static_cast<uint32_t>(_uploadInstances[i].size());
        offsetInstance += _indirectInstanceInfoPerMaterial[i].count;
    }

    for(uint32_t i = 0; i < _uploadIndirectCommands.size(); i++)
    {
        if(_uploadIndirectCommands[i].size() == 0) continue;

        uint32_t offset = _indirectDrawInfoPerMaterial[i].offset;
        for(auto& indirect : _uploadIndirectCommands[i])
        {
            indirect.firstInstance += offset;
        }
        _indirectBuffer.CopyToBuffer(queue, cmdPool, _uploadIndirectCommands[i].data(), _indirectDrawInfoPerMaterial[i].count * _indirectBuffer.GetStride());
    }



    for(uint32_t i = 0; i < _uploadInstances.size(); i++)
    {
        uint32_t offset = _indirectDrawInfoPerMaterial[i].offset;

        for(auto& instance : _uploadInstances[i])
        {
            instance.MeshIndex += offset;
        }

        if(_uploadInstances[i].size() != 0)
        {
        _instanceDataBuffer.CopyToBuffer(queue, cmdPool, _uploadInstances[i].data(), _indirectInstanceInfoPerMaterial[i].count * _instanceDataBuffer.GetStride());
        }
        _indirectDrawInfoPerMaterial[i].offset *= _indirectBuffer.GetStride();
    }


    _uploadVertices.clear();
    _uploadIndices.clear();
    for(auto& indirect : _uploadIndirectCommands)
    {
        indirect.clear();
    }
    for(auto& instances : _uploadInstances)
    {
        instances.clear();
    }
}

void MeshRegistry::Bind(const CommandBuffer& cmdBuffer, vk::PipelineLayout layout) const
{
    cmdBuffer.BindBuffer(_vertexBuffer);
    cmdBuffer.BindBuffer(_indexBuffer);
}
