#pragma once

#include "asset/Importer.h"
#include "asset/Vertex.h"
#include "allocation/MaterialRegistry.h"
#include "allocation/MeshRegistry.h"
#include "allocation/BindingManager.h"
#include <cstddef>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.hpp>

struct AssetImporter : public Importer
{
    struct Header
    {
        uint64_t MaterialCount;
        uint64_t TextureCount;
        uint64_t TextureDataCount;
        uint64_t SubMeshCount;
        uint64_t VertexCount;
        uint64_t IndexCount;
    };

    const static constexpr uint32_t VERSION = 5;

    Header Header;

    std::vector<MaterialImport> Materials;
    std::vector<TextureImport> Textures;
    std::vector<SubMeshImport> SubMeshes;
    std::vector<std::byte> TextureDatas;
    std::vector<Vertex> VertexDatas;
    std::vector<uint32_t> IndexDatas;

    AssetImporter() = default;
    AssetImporter(AssetImporter&& other) noexcept = default;
    AssetImporter& operator=(AssetImporter&& other) noexcept = default;
    ~AssetImporter() = default;

    static void LoadAsset(BindingManager& allocator, const std::filesystem::path & modelPath, MeshRegistry & meshRegistry, MaterialRegistry & materialRegistry, vk::CommandPool uploadPool, vk::Queue uploadQueue, vk::Sampler sampler);
    bool Load(const std::filesystem::path& path) override;
    void Clear() override;
};
