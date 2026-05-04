#pragma once

#include "glm/ext/vector_float4.hpp"
#include "registry/Registry.h"
#include "rendering/DescriptorPool.h"
#include "rendering/GPUImage.h"
#include <cstdint>

struct MaterialProperties
{
    alignas(4) uint32_t BaseColorTexId;
    alignas(4) uint32_t NormalMapTexId;
    alignas(4) uint32_t MRAOTexId;
    alignas(4) float Metallic;

    alignas(16) glm::vec4 BaseColor;
    alignas(4) float Roughness;

    static constexpr uint32_t Stride();

    static MaterialProperties Default()
    {
        return
        {
            .BaseColorTexId = UINT32_MAX,
            .NormalMapTexId = UINT32_MAX,
            .MRAOTexId = UINT32_MAX,
            .Metallic = 0.0f,
            .BaseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .Roughness = 1.0f
        };
    }

    bool operator==(const MaterialProperties& other) const
    {
        return BaseColor == other.BaseColor;
    }
};

inline constexpr uint32_t MaterialProperties::Stride() { return sizeof(MaterialProperties); }

class MaterialRegistry : public Registry
{
public:

    MaterialRegistry(DescriptorPool& pool, uint32_t layoutIndex, uint32_t maxTexturesCount = ApplicationInfo::Constant::MaxTextureCount);
    ~MaterialRegistry();

    void UploadAndRelease(VkQueue queue, VkCommandPool cmdPool) override;
    void Bind(const CommandBuffer& cmdBuffer, VkPipelineLayout layout) const override;

    uint32_t RegisterMaterial(MaterialProperties material);
    uint32_t RegisterTexture(GPUAllocatedImage&& texture, VkSampler sampler);
    uint32_t RemoveTexture(uint32_t textureIndex);

    inline DescriptorSetHandle BindlessTextureSet() const { return _bindlessTextureSet; }
    inline uint32_t MaxTexturesCount() const { return _maxTexturesCount; }
    inline uint32_t LastFreeTextureIndex() const { return _lastFreeTextureIndex; }
    inline GPUAllocatedImage& Texture(uint32_t index) { return _textures[index]; }

    inline MaterialProperties Material(uint32_t index) { return _materials[index]; }
    inline MaterialProperties DefaultMaterial() const { return _materials[0]; }
    inline uint32_t MaterialsCount() const { return _materials.size(); }

    VkDescriptorBufferInfo MaterialBufferInfo() const { return _materialBuffer.GetDescriptorBufferInfo(0, _materialBuffer.GetCount()); }

    private:
    uint32_t _maxTexturesCount;
    std::vector<int32_t> _textureIndicesState;
    uint32_t _lastFreeTextureIndex = 0;
    DescriptorSetHandle _bindlessTextureSet = {};

    std::vector<MaterialProperties> _materials;

    std::vector<GPUAllocatedImage> _textures;
    GeometryBuffer _materialBuffer;
};
