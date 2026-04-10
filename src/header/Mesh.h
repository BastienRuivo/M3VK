#pragma once

#include "header/GraphicsBuffer.h"
#include <cstddef>
#include <cstdint>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>
class Mesh
{
    public:
    Mesh();
    ~Mesh();

    void LoadFromObj(const std::string& path);

    void UploadAndRelease(const VkPhysicalDeviceHandler& physicalDevice, VkDevice device, VkQueue queue, VkCommandPool cmdPool, MemoryBuffer& vertexBuffer, MemoryBuffer& indexBuffer);
    void Dispose();

    void SetVertexBufferSize(uint32_t count);
    void SetIndexBufferSize(uint32_t count);

    VkVertexInputBindingDescription GetBindingDescription() const;
    inline uint32_t GetVertexCount() const { return _vertexCount; }
    inline uint32_t GetIndexCount() const { return _indexCount; }

    void* GetVertexData() const { return _vertexData; }

    template<typename T>
    void* GetIndexData() const { return _indexData; }

    private:
    uint32_t _vertexCount = 0;
    std::byte* _vertexData;

    uint32_t _indexCount = 0;
    std::byte* _indexData;
};
