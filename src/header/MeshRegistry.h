#pragma once

#include "header/ApplicationInfo.h"
#include "header/CommandBuffer.h"
#include "header/GraphicsBuffer.h"
#include "header/Vertex.h"
#include <cstdint>
#include <span>
#include <vector>
struct SubMesh
{
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t vertexOffset;
};

class MeshRegistry
{
    public:
    MeshRegistry(VkDevice device, const VkPhysicalDeviceHandler& physicalDevice, uint32_t vertexBufferSize = ApplicationInfo::Constant::VertexBufferMaxSize, uint32_t indexBufferSize = ApplicationInfo::Constant::IndexBufferMaxSize);

    SubMesh Add(std::span<const Vertex> vertices, std::span<const uint32_t> indices);
    SubMesh AddFromObj(const std::string& path);
    void UploadAndRelease(const VkPhysicalDeviceHandler& physicalDevice, VkDevice device, VkQueue queue, VkCommandPool cmdPool);

    void Bind(const CommandBuffer& cmdBuffer) const;

    private:
    std::vector<Vertex> _cpuVertices;
    std::vector<uint32_t> _cpuIndices;

    GeometryBuffer _vertexBuffer;
    GeometryBuffer _indexBuffer;
};
