

#include <cstdio>

#include "asset/AssetImporter.h"
#include "application/ApplicationHelper.h"
#include "application/DebugLayer.h"
#include "asset/ImporterHelper.h"
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

bool AssetImporter::Load(const std::filesystem::path& path)
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

    fread(&Version, sizeof(uint32_t), 1, file);

    if(Version != VERSION)
    {
        DebugLayer::Log(DebugLayer::LogType::WARNING, "File '" + path.string() + "' version : " + std::to_string(Version) + " != " + std::to_string(VERSION));
        return false;
    }

    fread(&Header, sizeof(Header), 1, file);

    // DATAS
    Materials.resize(Header.MaterialCount);
    fread(Materials.data(), sizeof(MaterialImport), Header.MaterialCount, file);

    Textures.resize(Header.TextureCount);
    fread(Textures.data(), sizeof(TextureImport), Header.TextureCount, file);

    SubMeshes.resize(Header.SubMeshCount);
    fread(SubMeshes.data(), sizeof(SubMeshImport), Header.SubMeshCount, file);

    VertexDatas.resize(Header.VertexCount);
    fread(VertexDatas.data(), sizeof(Vertex), Header.VertexCount, file);

    IndexDatas.resize(Header.IndexCount);
    fread(IndexDatas.data(), sizeof(uint32_t), Header.IndexCount, file);

    TextureDatas.resize(Header.TextureDataCount);
    fread(TextureDatas.data(), sizeof(std::byte), Header.TextureDataCount, file);

    fclose(file);

    return true;
}

void AssetImporter::Clear()
{
    Materials.clear();
    Textures.clear();
    SubMeshes.clear();
    VertexDatas.clear();
    IndexDatas.clear();
    TextureDatas.clear();
}

void AssetImporter::LoadAsset(DescriptorAllocator& allocator, const std::filesystem::path & modelPath, MeshRegistry & meshRegistry, MaterialRegistry & materialRegistry, VkCommandPool uploadPool, VkQueue uploadQueue, VkSampler sampler)
{
    AssetImporter importer;

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    if(!importer.Load(modelPath))
    {
        return ;
    }

    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

    DebugLayer::Log(DebugLayer::LogType::INFO, "AssetImporter load time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()) + "ms");

    uint32_t materialOffset = materialRegistry.MaterialsCount();
    {
        PoolStageBuffer uploadBuffer(4096 * 4096 * 4, StageBuffer::Usage::Upload);
        uploadBuffer.Map();
        std::array<ImporterHelper::UploadCommand, 16> uploadCommands;
        uint32_t uploadCommandCount = 0;

        MaterialProperties defaultMaterial = materialRegistry.DefaultMaterial();

        uint32_t materialOffset = materialRegistry.MaterialsCount();
        for(unsigned int i = 0; i < importer.Header.MaterialCount; i++)
        {
            const auto & material = importer.Materials[i];
            MaterialProperties gpuMaterial = MaterialProperties {
                .BaseColorTexId = material.BaseColorTexId,
                .NormalMapTexId = material.NormalMapTexId,
                .MRAOTexId = material.MRAOTexId,
                .MaterialType = material.MaterialType,
                .BaseColor = material.BaseColor,
                .Metallic = material.Metallic,
                .Roughness = material.Roughness
            };

            gpuMaterial.BaseColorTexId = material.BaseColorTexId == UINT32_MAX ? defaultMaterial.BaseColorTexId: ImporterHelper::LoadTexture(allocator, importer.Textures, importer.TextureDatas, material.BaseColorTexId, materialRegistry, uploadBuffer, uploadCommands.data(), uploadCommandCount, sampler, uploadPool, uploadQueue);
            gpuMaterial.NormalMapTexId = material.NormalMapTexId == UINT32_MAX ? defaultMaterial.NormalMapTexId : ImporterHelper::LoadTexture(allocator, importer.Textures, importer.TextureDatas, material.NormalMapTexId, materialRegistry, uploadBuffer, uploadCommands.data(), uploadCommandCount, sampler, uploadPool, uploadQueue);
            gpuMaterial.MRAOTexId = material.MRAOTexId == UINT32_MAX ? defaultMaterial.MRAOTexId : ImporterHelper::LoadTexture(allocator, importer.Textures, importer.TextureDatas, material.MRAOTexId, materialRegistry, uploadBuffer, uploadCommands.data(), uploadCommandCount, sampler, uploadPool, uploadQueue);

            materialRegistry.RegisterMaterial(gpuMaterial);
        }

        ImporterHelper::UploadTexture(uploadCommands.data(), uploadCommandCount, uploadBuffer, uploadQueue, uploadPool);
        uploadBuffer.Unmap();
    }

    std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();

    DebugLayer::Log(DebugLayer::LogType::INFO, "AssetImporter material load time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count()) + "ms");

    std::vector<MeshHandle> subMeshes;

    uint32_t meshCount = importer.Header.SubMeshCount;
    for(unsigned int i = 0; i < meshCount; i++)
    {
        const auto & mesh = importer.SubMeshes[i];

        std::span<const Vertex> vertices(importer.VertexDatas.data() + mesh.VertexOffset, mesh.VertexCount);
        std::span<const uint32_t> indices(importer.IndexDatas.data() + mesh.IndexOffset, mesh.IndexCount);

        MaterialProperties material = materialRegistry.Material(mesh.MaterialIndex + materialOffset);
        uint32_t submesh = meshRegistry.RegisterMesh(static_cast<MaterialType>(material.MaterialType), vertices, indices);
        InstanceData instance = {
            .LocalToWorldMatrix = ApplicationHelper::TranslateRotateScale(glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(1.0f, 1.0f, 1.0f)),
            .AabbMin = mesh.AABBMin,
            .MaterialIndex = materialOffset + mesh.MaterialIndex,
            .AabbMax = mesh.AABBMax,
            .MeshIndex = submesh
        };

        meshRegistry.RegisterInstance(static_cast<MaterialType>(material.MaterialType), instance);
    }

    std::chrono::high_resolution_clock::time_point t4 = std::chrono::high_resolution_clock::now();
    DebugLayer::Log(DebugLayer::LogType::INFO, "AssetImporter mesh load time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count()) + "ms");
    DebugLayer::Log(DebugLayer::LogType::INFO, "AssetImporter load time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t1).count()) + "ms -------------------------------------------");
}
