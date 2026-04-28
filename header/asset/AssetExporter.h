#pragma once

#include "asset/Vertex.h"
#include "assimp/material.h"
#include "registry/MaterialRegistry.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>
#include <vulkan/vulkan_core.h>

#define VERSION 0

struct TextureExport
{
    size_t Offset;
    size_t Size;
    size_t Width;
    size_t Height;
    VkFormat Format;
    uint32_t MipCount;
};

struct MaterialExport
{
    uint32_t BaseColorTexId;
    uint32_t NormalMapTexId;
    uint32_t MRAOTexId;
    GPUMaterial MaterialProperties;
};

struct SubMeshExport
{
    uint32_t MaterialIndex;
    uint32_t VertexOffset;
    uint32_t VertexCount;
    uint32_t IndexOffset;
    uint32_t IndexCount;
};

struct AssetExporterHeader
{
    uint64_t MaterialCount;
    uint64_t TextureCount;
    uint64_t TextureDataCount;
    uint64_t SubMeshCount;
    uint64_t VertexCount;
    uint64_t IndexCount;
};

struct AssetExporter
{
    std::filesystem::path ExportPath;

    uint32_t Version;
    AssetExporterHeader Header;

    std::vector<MaterialExport> Materials;
    std::vector<TextureExport> Textures;
    std::vector<SubMeshExport> SubMeshes;
    std::vector<std::byte> TextureDatas;
    std::vector<Vertex> VertexDatas;
    std::vector<uint32_t> IndexDatas;

    AssetExporter() = default;
    AssetExporter(AssetExporter&& other) noexcept;
    AssetExporter& operator=(AssetExporter&& other) noexcept;

    static AssetExporter Load3DModel(const std::string & modelPath, VkCommandPool uploadPool, VkQueue uploadQueue);
    static uint32_t LoadTexture(const aiMaterial* material, const std::filesystem::path rootPath, std::span<const aiTextureType> types, std::vector<std::byte>& textureDatas, std::vector<TextureExport>& textures, VkCommandPool uploadPool, VkQueue uploadQueue);
    static void Write(const AssetExporter& exporter);
    static bool Load(AssetExporter& exporter);
    static void Clear(AssetExporter& exporter);
};
