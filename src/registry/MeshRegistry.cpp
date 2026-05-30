#include "registry/MeshRegistry.h"
#include "application/ApplicationInfo.h"
#include <cassert>
#include <cstdint>
#include <vulkan/vulkan_core.h>

#include "../shaders/Shader_Bindings.h"

MeshRegistry::MeshRegistry(DescriptorAllocator& allocator, size_t vertexBufferSize, size_t indexBufferSize, size_t indirectBufferSize)
: _vertexBuffer(GraphicsBuffer::BufferType::VERTEX, RessourceUsage::Static, vertexBufferSize, sizeof(Vertex)),
    _indexBuffer(GraphicsBuffer::BufferType::INDEX, RessourceUsage::Static, indexBufferSize, sizeof(uint32_t)),
    _indirectBuffer(GraphicsBuffer::BufferType::INDIRECT_DRAW, RessourceUsage::Static, indirectBufferSize, sizeof(DrawIndexedIndirectPadded)),
    _instanceDataBuffer(allocator, STATIC_BINDING_INSTANCE_DATA_BUFFER, GraphicsBuffer::BufferType::STORAGE, RessourceUsage::Static, indirectBufferSize, sizeof(InstanceData))
{
}

uint32_t MeshRegistry::RegisterMesh(std::span<const Vertex> vertices, std::span<const uint32_t> indices)
{
    MeshHandle subMesh
    {
        .firstIndex = static_cast<uint32_t>(_cpuIndices.size()),
        .indexCount = static_cast<uint32_t>(indices.size()),
        .firstVertex = static_cast<uint32_t>(_cpuVertices.size())
    };

    _cpuVertices.insert(_cpuVertices.end(), vertices.begin(), vertices.end());
    _cpuIndices.insert(_cpuIndices.end(), indices.begin(), indices.end());
    _cpuIndirectCommands.push_back(DrawIndexedIndirectPadded
    {
        .indexCount = subMesh.indexCount,
        .instanceCount = 0,
        .firstIndex = subMesh.firstIndex,
        .vertexOffset = static_cast<int32_t>(subMesh.firstVertex),
        .firstInstance = static_cast<uint32_t>(_cpuInstances.size())
    });

    return static_cast<uint32_t>(_cpuIndirectCommands.size() - 1);
}

uint32_t MeshRegistry::RegisterInstance(InstanceData instances)
{
    instances.meshId = static_cast<uint32_t>(_cpuIndirectCommands.size() - 1);
    _cpuInstances.push_back(instances);
    _cpuIndirectCommands.back().instanceCount++;
    return static_cast<uint32_t>(_cpuInstances.size() - 1);
}

DrawIndexedIndirectPadded& MeshRegistry::RegisterIndirectCommand(DrawIndexedIndirectPadded command)
{
    _cpuIndirectCommands.push_back(command);
    return _cpuIndirectCommands.back();
}

void MeshRegistry::UploadAndRelease(VkQueue queue, VkCommandPool cmdPool)
{
    assert(_cpuVertices.size() <= ApplicationInfo::Constant::VertexBufferMaxSize);
    assert(_cpuIndices.size() <= ApplicationInfo::Constant::IndexBufferMaxSize);
    assert(_cpuInstances.size() <= ApplicationInfo::Constant::DrawIndirectBufferMaxSize);

    if(_cpuIndices.size() == 0 || _cpuVertices.size() == 0 || _cpuInstances.size() == 0)
    {
        DebugLayer::Log(DebugLayer::LogType::WARNING, "No vertices or indices or instances to upload");
        return;
    }

    _vertexBuffer.CopyToBuffer(queue, cmdPool, _cpuVertices.data(), _cpuVertices.size() * _vertexBuffer.GetStride());
    _indexBuffer.CopyToBuffer(queue, cmdPool, _cpuIndices.data(), _cpuIndices.size() * _indexBuffer.GetStride());
    _indirectBuffer.CopyToBuffer(queue, cmdPool, _cpuIndirectCommands.data(), _cpuIndirectCommands.size() * _indirectBuffer.GetStride());
    _instanceDataBuffer.CopyToBuffer(queue, cmdPool, _cpuInstances.data(), _cpuInstances.size() * _instanceDataBuffer.GetStride());

    _cpuVertices.clear();
    _cpuIndices.clear();
    _cpuIndirectCommands.clear();
    _cpuInstances.clear();
}

void MeshRegistry::Bind(const CommandBuffer& cmdBuffer, VkPipelineLayout layout) const
{
    cmdBuffer.BindBuffer(_vertexBuffer);
    cmdBuffer.BindBuffer(_indexBuffer);
}
