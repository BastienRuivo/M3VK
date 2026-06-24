#pragma once

#include "asset/Importer.h"
#include "registry/MaterialRegistry.h"
#include "rendering/DescriptorAllocator.h"
#include <cstddef>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

struct MaterialImporter : public Importer
{
    struct Header
    {
        uint64_t TextureCount;
        uint64_t TextureDataCount;
        MaterialImport Material;
    };

    const static constexpr uint32_t VERSION = 0;

    Header Header;

    std::vector<TextureImport> Textures;
    std::vector<std::byte> TextureDatas;

    MaterialImporter() = default;
    MaterialImporter(MaterialImporter&& other) noexcept;
    MaterialImporter& operator=(MaterialImporter&& other) noexcept;
    ~MaterialImporter() = default;

    static uint32_t LoadMaterial(DescriptorAllocator& allocator, const std::string & modelPath, MaterialRegistry & materialRegistry, VkCommandPool uploadPool, VkQueue uploadQueue, VkSampler sampler);
    static uint32_t LoadDefaultMaterial(DescriptorAllocator& allocator, MaterialRegistry & materialRegistry, VkCommandPool uploadPool, VkQueue uploadQueue, VkSampler sampler);
    bool Load(const std::filesystem::path& path) override;
    void Clear() override;
};
