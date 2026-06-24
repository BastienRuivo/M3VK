#pragma once

#include "Material.h"
#include "registry/Registry.h"
#include "rendering/DescriptorAllocator.h"
#include "rendering/GPUImage.h"
#include <cstdint>

class MaterialRegistry : public Registry
{
public:

    MaterialRegistry(DescriptorAllocator& pool, uint32_t maxTexturesCount, VkCommandPool uploadPool, VkQueue uploadQueue, VkSampler sampler);
    ~MaterialRegistry();

    void UploadAndRelease(VkQueue queue, VkCommandPool cmdPool) override;
    void Bind(const CommandBuffer& cmdBuffer, VkPipelineLayout layout) const override;

    uint32_t RegisterMaterial(MaterialProperties material);
    uint32_t RegisterTexture(DescriptorAllocator& allocator, GPUImage&& texture, VkSampler sampler);
    uint32_t RemoveTexture(uint32_t textureIndex);

    inline uint32_t MaxTexturesCount() const { return _maxTexturesCount; }
    inline uint32_t LastFreeTextureIndex() const { return _lastFreeTextureIndex; }
    inline GPUImage& Texture(uint32_t index) { return _textures[index]; }

    inline MaterialProperties Material(uint32_t index) { return _materials[index]; }
    inline MaterialProperties DefaultMaterial() const { return _materials[_defaultMaterialIndex]; }
    inline uint32_t DefaultMaterialIndex() const { return _defaultMaterialIndex; }
    inline uint32_t MaterialsCount() const { return _materials.size(); }

    inline uint32_t GetMaterialBufferGPUIndex() const { return _materialBuffer.GetGPUIndex(); }

    private:
    uint32_t _maxTexturesCount;
    std::vector<int32_t> _textureIndicesState;
    uint32_t _lastFreeTextureIndex = 0;

    std::vector<MaterialProperties> _materials;
    uint32_t _defaultMaterialIndex;

    std::vector<GPUImage> _textures;
    GeometryBuffer _materialBuffer;
};
