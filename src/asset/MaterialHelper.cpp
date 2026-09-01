

#include <cstdio>

#include "Material.h"
#include "asset/MaterialImporter.h"
#include "application/DebugLayer.h"
#include "asset/ImporterHelper.h"
#include "allocation/BindingManager.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <vulkan/vulkan.hpp>

bool MaterialImporter::Load(const std::filesystem::path& path)
{
    if(path.empty() || !std::filesystem::exists(path))
    {
        DebugLayer::Log(DebugLayer::LogType::WARNING, "File '" + path.string() + "' does not exist");
        return false;
    }
    else if (path.extension() != ".m3vkmaterial")
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

    Textures.resize(Header.TextureCount);
    fread(Textures.data(), sizeof(TextureImport), Header.TextureCount, file);

    TextureDatas.resize(Header.TextureDataCount);
    fread(TextureDatas.data(), sizeof(std::byte), Header.TextureDataCount, file);

    fclose(file);

    return true;
}

void MaterialImporter::Clear()
{
    Textures.clear();
    TextureDatas.clear();
}

uint32_t LoadMaterialInternal(BindingManager& allocator, MaterialImporter& importer, const MaterialProperties& defaultMaterial, MaterialRegistry & materialRegistry, vk::CommandPool uploadPool, vk::Queue uploadQueue, vk::Sampler sampler)
{
    uint32_t materialOffset = materialRegistry.MaterialsCount();
    PoolStageBuffer uploadBuffer(4096 * 4096 * 4, StageBuffer::Usage::Upload);
    uploadBuffer.Map();
    std::array<ImporterHelper::UploadCommand, 16> uploadCommands;
    uint32_t uploadCommandCount = 0;

    const auto & material = importer.Header.Material;
    MaterialProperties gpuMaterial = MaterialProperties
    {
        .BaseColorTexId = material.BaseColorTexId,
        .NormalMapTexId = material.NormalMapTexId,
        .MRAOTexId = material.MRAOTexId,
        .MaterialType = material.MaterialType,
        .BaseColor = material.BaseColor,
        .Metallic = material.Metallic,
        .Roughness = material.Roughness
    };

    gpuMaterial.BaseColorTexId = material.BaseColorTexId == UINT32_MAX ? defaultMaterial.BaseColorTexId : ImporterHelper::LoadTexture(allocator, importer.Textures, importer.TextureDatas, material.BaseColorTexId, materialRegistry, uploadBuffer, uploadCommands.data(), uploadCommandCount, sampler, uploadPool, uploadQueue).StaticIndex();
    gpuMaterial.NormalMapTexId = material.NormalMapTexId == UINT32_MAX ? defaultMaterial.NormalMapTexId : ImporterHelper::LoadTexture(allocator, importer.Textures, importer.TextureDatas, material.NormalMapTexId, materialRegistry, uploadBuffer, uploadCommands.data(), uploadCommandCount, sampler, uploadPool, uploadQueue).StaticIndex();
    gpuMaterial.MRAOTexId = material.MRAOTexId == UINT32_MAX ? defaultMaterial.MRAOTexId : ImporterHelper::LoadTexture(allocator, importer.Textures, importer.TextureDatas, material.MRAOTexId, materialRegistry, uploadBuffer, uploadCommands.data(), uploadCommandCount, sampler, uploadPool, uploadQueue).StaticIndex();

    uint32_t materialId = materialRegistry.RegisterMaterial(gpuMaterial);

    ImporterHelper::UploadTexture({uploadCommands.data(), uploadCommandCount}, uploadBuffer, uploadQueue, uploadPool);
    uploadBuffer.Unmap();

    return materialId;
}

uint32_t MaterialImporter::LoadMaterial(BindingManager& allocator, const std::filesystem::path & modelPath, MaterialRegistry & materialRegistry, vk::CommandPool uploadPool, vk::Queue uploadQueue, vk::Sampler sampler)
{
    MaterialImporter importer;

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    if(!importer.Load(modelPath))
    {
        return materialRegistry.DefaultMaterialIndex();
    }

    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

    DebugLayer::Log(DebugLayer::LogType::INFO, "MaterialImporter load time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()) + "ms");

    uint32_t materialId = LoadMaterialInternal(allocator, importer, materialRegistry.DefaultMaterial(), materialRegistry, uploadPool, uploadQueue, sampler);

    std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();
    DebugLayer::Log(DebugLayer::LogType::INFO, "MaterialImporter load time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t1).count()) + "ms -------------------------------------------");
    return materialId;
}

uint32_t MaterialImporter::LoadDefaultMaterial(BindingManager& allocator, MaterialRegistry & materialRegistry, vk::CommandPool uploadPool, vk::Queue uploadQueue, vk::Sampler sampler)
{
    MaterialImporter importer;

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    if(!importer.Load("data/DefaultMTL.m3vkmaterial"))
    {
        throw std::runtime_error("Failed to load default material");
    }

    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

    DebugLayer::Log(DebugLayer::LogType::INFO, "MaterialImporter load time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()) + "ms");

    MaterialProperties defaultMaterial = {};
    uint32_t materialId = LoadMaterialInternal(allocator, importer, defaultMaterial, materialRegistry, uploadPool, uploadQueue, sampler);

    std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();
    DebugLayer::Log(DebugLayer::LogType::INFO, "MaterialImporter load time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t1).count()) + "ms -------------------------------------------");

    return materialId;
}
