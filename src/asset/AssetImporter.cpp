

#include <cstdio>

#include "asset/AssetImporter.h"
#include "application/DebugLayer.h"
#include "registry/MaterialRegistry.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <vulkan/vulkan_core.h>

AssetImporter::AssetImporter(AssetImporter&& other) noexcept
{
    Header = std::move(other.Header);
    Materials = std::move(other.Materials);
    Textures = std::move(other.Textures);
    SubMeshes = std::move(other.SubMeshes);
    TextureDatas = std::move(other.TextureDatas);
    VertexDatas = std::move(other.VertexDatas);
    IndexDatas = std::move(other.IndexDatas);
}

AssetImporter& AssetImporter::operator=(AssetImporter&& other) noexcept
{
    if(this != &other)
    {
        Header = std::move(other.Header);
        Materials = std::move(other.Materials);
        Textures = std::move(other.Textures);
        SubMeshes = std::move(other.SubMeshes);
        TextureDatas = std::move(other.TextureDatas);
        VertexDatas = std::move(other.VertexDatas);
        IndexDatas = std::move(other.IndexDatas);
    }
    return *this;
}

bool AssetImporter::Load(AssetImporter& importer, const std::filesystem::path& path)
{
    if(path.empty() || !std::filesystem::exists(path))
    {
        DebugLayer::Log(DebugLayer::LogType::WARNING, "File '" + path.string() + "' does not exist");
        return false;
    }
    else if (path.extension() != ".m3vkasset")
    {
        DebugLayer::Log(DebugLayer::LogType::WARNING, "File '" + path.string() + "' is not a valid .m3vkasset file");
        return false;
    }

    auto file = fopen(path.string().c_str(), "rb");

    if(file == nullptr)
    {
        DebugLayer::Log(DebugLayer::LogType::WARNING, "Failed to open file " + path.string());
        return false;
    }

    fread(&importer.Version, sizeof(uint32_t), 1, file);

    if(importer.Version < VERSION)
    {
        DebugLayer::Log(DebugLayer::LogType::WARNING, "File '" + path.string() + "' version is too old : " + std::to_string(importer.Version) + " < " + std::to_string(VERSION));
        return false;
    }

    fread(&importer.Header, sizeof(AssetHeader), 1, file);

    // DATAS
    importer.Materials.resize(importer.Header.MaterialCount);
    fread(importer.Materials.data(), sizeof(MaterialProperties), importer.Header.MaterialCount, file);

    importer.Textures.resize(importer.Header.TextureCount);
    fread(importer.Textures.data(), sizeof(TextureImport), importer.Header.TextureCount, file);

    importer.SubMeshes.resize(importer.Header.SubMeshCount);
    fread(importer.SubMeshes.data(), sizeof(SubMeshImport), importer.Header.SubMeshCount, file);

    importer.VertexDatas.resize(importer.Header.VertexCount);
    fread(importer.VertexDatas.data(), sizeof(Vertex), importer.Header.VertexCount, file);

    importer.IndexDatas.resize(importer.Header.IndexCount);
    fread(importer.IndexDatas.data(), sizeof(uint32_t), importer.Header.IndexCount, file);

    importer.TextureDatas.resize(importer.Header.TextureDataCount);
    fread(importer.TextureDatas.data(), sizeof(std::byte), importer.Header.TextureDataCount, file);

    fclose(file);

    return true;
}

void AssetImporter::Clear(AssetImporter& importer)
{
    importer.Materials.clear();
    importer.Textures.clear();
    importer.SubMeshes.clear();
    importer.VertexDatas.clear();
    importer.IndexDatas.clear();
    importer.TextureDatas.clear();
}
