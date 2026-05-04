#pragma once

#include "asset/Vertex.h"
#include "registry/MaterialRegistry.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>
#include <vulkan/vulkan_core.h>

#define VERSION 0

enum TextureType
{
    BaseColor = 0,
    NormalMap = 1,
    MRAO = 2
};

struct TextureImport
{
    uint32_t Type;
    uint32_t Offset;
    uint32_t Size;
    uint32_t Width;
    uint32_t Height;
    VkFormat Format;
    uint32_t MipCount;
};

struct SubMeshImport
{
    uint32_t MaterialIndex;
    uint32_t VertexOffset;
    uint32_t VertexCount;
    uint32_t IndexOffset;
    uint32_t IndexCount;
};

struct AssetHeader
{
    uint64_t MaterialCount;
    uint64_t TextureCount;
    uint64_t TextureDataCount;
    uint64_t SubMeshCount;
    uint64_t VertexCount;
    uint64_t IndexCount;
};

struct AssetImporter
{
    uint32_t Version;
    AssetHeader Header;

    std::vector<MaterialProperties> Materials;
    std::vector<TextureImport> Textures;
    std::vector<SubMeshImport> SubMeshes;
    std::vector<std::byte> TextureDatas;
    std::vector<Vertex> VertexDatas;
    std::vector<uint32_t> IndexDatas;

    std::vector<std::byte> UncompressedDataCache;

    AssetImporter() = default;
    AssetImporter(AssetImporter&& other) noexcept;
    AssetImporter& operator=(AssetImporter&& other) noexcept;

    static bool Load(AssetImporter& exporter, const std::filesystem::path& path);
    static void Clear(AssetImporter& exporter);
};
