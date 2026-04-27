#pragma once

#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "application/ApplicationInfo.h"
#include "registry/Registry.h"
#include "rendering/BufferHelper.h"

struct GPUMaterial
{
    alignas(16) glm::vec4 BaseColor;
    alignas(4) float Metallic;
    alignas(4) float Roughness;

    static GPUMaterial Default()
    {
        return
        {
            .BaseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .Metallic = 0.0f,
            .Roughness = 1.0f
        };
    }

    bool operator==(const GPUMaterial& other) const
    {
        return BaseColor == other.BaseColor;
    }
};

class MaterialRegistry : public Registry
{
    public:
    MaterialRegistry(size_t materialBufferSize = ApplicationInfo::Constant::MaterialBufferMaxSize);

    BufferHelper::BufferBinding Register(GPUMaterial material);

    void UploadAndRelease(VkQueue queue, VkCommandPool cmdPool) override;
    void Bind(const CommandBuffer& cmdBuffer) const override;

    private:
    std::vector<GPUMaterial> _materials;
    GeometryBuffer _materialBuffer;
};
