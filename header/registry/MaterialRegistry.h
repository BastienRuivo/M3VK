#pragma once

#include "glm/ext/vector_float4.hpp"
#include "application/ApplicationInfo.h"
#include "registry/Registry.h"
#include "rendering/BufferHelper.h"

struct GPUMaterial
{
    alignas(16)glm::vec4 Albedo;

    static GPUMaterial Default()
    {
        return
        {
            .Albedo = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
        };
    }

    bool operator==(const GPUMaterial& other) const
    {
        return Albedo == other.Albedo;
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
