#include "header/Mesh.h"
#include "header/DebugLayer.h"
#include "header/GraphicsBuffer.h"
#include "header/VkHandlers/VkPhysicalDeviceHandler.h"
#include <vulkan/vulkan_core.h>

Mesh::Mesh() {}

Mesh::~Mesh()
{
    if(_vertexData != nullptr)
    {
        DebugLayer::Log(DebugLayer::LogType::WARNING, "Mesh destroying vertex data in the destructor, was this mesh uploaded ?");
        delete _vertexData;
    }

    if(_indexData != nullptr)
    {
        DebugLayer::Log(DebugLayer::LogType::WARNING, "Mesh destroying index data in the destructor, was this mesh uploaded ?");
        delete _indexData;
    }
}

void Mesh::UploadAndRelease(const VkPhysicalDeviceHandler& physicalDevice, VkDevice device, VkQueue queue, VkCommandPool cmdPool, MemoryBuffer& vertexBuffer, MemoryBuffer& indexBuffer)
{
    if(_indexData != nullptr)
    {
        indexBuffer.CopyToBuffer(physicalDevice, device, queue, cmdPool, _indexData, _indexCount * indexBuffer.GetStride());
    }

    if(_vertexData != nullptr)
    {
        vertexBuffer.CopyToBuffer(physicalDevice, device, queue, cmdPool, _vertexData, _vertexCount * vertexBuffer.GetStride());
    }

    Dispose();
}

void Mesh::Dispose()
{
    if(_vertexData != nullptr)
    {
        delete _vertexData;
        _vertexData = nullptr;
    }

    if(_indexData != nullptr)
    {
        delete _indexData;
        _indexData = nullptr;
    }
}

void Mesh::SetVertexBufferSize(uint32_t count)
{
    _vertexCount = count;
    _vertexData = new std::byte[count * sizeof(Vertex)];
}

void Mesh::SetIndexBufferSize(uint32_t count)
{
    _indexCount = count;
    _indexData = new std::byte[count * sizeof(uint32_t)];
}

void Mesh::LoadFromObj(const std::string& path)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    ProjectHelper::LoadObj(path, vertices, indices);

    SetVertexBufferSize(vertices.size());
    SetIndexBufferSize(indices.size());
    memcpy(_vertexData, vertices.data(), vertices.size() * sizeof(Vertex));
    memcpy(_indexData, indices.data(), indices.size() * sizeof(uint32_t));
}
