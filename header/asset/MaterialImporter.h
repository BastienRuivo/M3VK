#pragma once

#include "asset/Importer.h"
#include "allocation/MaterialRegistry.h"
#include "allocation/BindingManager.h"
#include <cstddef>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.hpp>

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
    MaterialImporter(MaterialImporter&& other) noexcept = default;
    MaterialImporter& operator=(MaterialImporter&& other) noexcept = default;
    ~MaterialImporter() = default;

    static uint32_t LoadMaterial(BindingManager& allocator, const std::filesystem::path & modelPath, MaterialRegistry & materialRegistry, vk::CommandPool uploadPool, vk::Queue uploadQueue, vk::Sampler sampler);
    static uint32_t LoadDefaultMaterial(BindingManager& allocator, MaterialRegistry & materialRegistry, vk::CommandPool uploadPool, vk::Queue uploadQueue, vk::Sampler sampler);
    bool Load(const std::filesystem::path& path) override;
    void Clear() override;
};
