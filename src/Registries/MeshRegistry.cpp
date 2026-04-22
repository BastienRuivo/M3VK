#include "header/Registries/MeshRegistry.h"
#include "header/ApplicationInfo.h"
#include <stdexcept>

MeshRegistry::MeshRegistry(size_t vertexBufferSize, size_t indexBufferSize)
: _vertexBuffer(vertexBufferSize, sizeof(Vertex), GraphicsBuffer::BufferType::VERTEX),
    _indexBuffer(indexBufferSize, sizeof(uint32_t), GraphicsBuffer::BufferType::INDEX)
{

}

SubMesh MeshRegistry::Register(std::span<const Vertex> vertices, std::span<const uint32_t> indices)
{
    SubMesh subMesh
    {
        .firstIndex = static_cast<uint32_t>(_cpuIndices.size()),
        .indexCount = static_cast<uint32_t>(indices.size()),
        .vertexOffset = static_cast<uint32_t>(_cpuVertices.size())
    };

    _cpuVertices.insert(_cpuVertices.end(), vertices.begin(), vertices.end());
    _cpuIndices.insert(_cpuIndices.end(), indices.begin(), indices.end());

    return subMesh;
}

void MeshRegistry::UploadAndRelease(VkQueue queue, VkCommandPool cmdPool)
{
    if(_cpuVertices.size() >= ApplicationInfo::Constant::VertexBufferMaxSize)
    {
        DebugLayer::Log(DebugLayer::LogType::ERROR, "Max vertex buffer size reached");
        throw std::runtime_error("Max vertex buffer size reached");
    }

    if(_cpuIndices.size() >= ApplicationInfo::Constant::IndexBufferMaxSize)
    {
        DebugLayer::Log(DebugLayer::LogType::ERROR, "Max index buffer size reached");
        throw std::runtime_error("Max index buffer size reached");
    }

    _vertexBuffer.CopyToBuffer(queue, cmdPool, _cpuVertices.data(), _cpuVertices.size() * sizeof(Vertex));
    _indexBuffer.CopyToBuffer(queue, cmdPool, _cpuIndices.data(), _cpuIndices.size() * sizeof(uint32_t));

    _cpuVertices.clear();
    _cpuIndices.clear();
}

void MeshRegistry::Bind(const CommandBuffer& cmdBuffer) const
{
    cmdBuffer.BindBuffer(_vertexBuffer);
    cmdBuffer.BindBuffer(_indexBuffer);
}
