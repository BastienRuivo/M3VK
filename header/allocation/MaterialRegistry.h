#pragma once

#include "Material.h"
#include "allocation/Registry.h"
#include "allocation/BindingManager.h"
#include <cstdint>

class MaterialRegistry : public Registry
{
public:

    MaterialRegistry(BindingManager& pool, VkCommandPool uploadPool, VkQueue uploadQueue, VkSampler sampler);
    ~MaterialRegistry();

    void UploadAndRelease(VkQueue queue, VkCommandPool cmdPool) override;
    void Bind(const CommandBuffer& cmdBuffer, VkPipelineLayout layout) const override;

    uint32_t RegisterMaterial(MaterialProperties material);

    inline MaterialProperties Material(uint32_t index) { return _materials[index]; }
    inline MaterialProperties DefaultMaterial() const { return _materials[_defaultMaterialIndex]; }
    inline uint32_t DefaultMaterialIndex() const { return _defaultMaterialIndex; }
    inline uint32_t MaterialsCount() const { return _materials.size(); }

    inline uint32_t GetMaterialBufferGPUIndex() const { return _materialBuffer.GetGPUIndex(); }

    private:
    std::vector<MaterialProperties> _materials;
    uint32_t _defaultMaterialIndex;
    GeometryBuffer _materialBuffer;
};
