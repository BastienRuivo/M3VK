#pragma once

#include "asset/AssetExporter.h"
#include "assimp/material.h"
#include "rendering/DescriptorPool.h"
#include "rendering/GPUImage.h"
#include "registry/MaterialRegistry.h"
#include "registry/MeshRegistry.h"
#include "rendering/Renderer.h"
#include <cstdint>
#include <filesystem>

namespace AssetHelper
{
    aiTextureType SelectTextureType(std::span<const aiTextureType> types, const aiMaterial* material, uint32_t& textureCount);
    ImageHelper::ImageBinding LoadTexture(AssetExporter& exporter, uint32_t textureIndex, std::vector<GPUAllocatedImage> & textures, VkSampler sampler, VkCommandPool uploadPool, VkQueue uploadQueue);
    Renderer Load3DModel(const std::string & modelPath, MeshRegistry & meshRegistry, MaterialRegistry & materialRegistry, std::vector<GPUAllocatedImage> & textures, std::vector<Material> & materials, int defaultMaterial, DescriptorPool& descriptorPool, VkCommandPool cmdPool, VkQueue queue, VkSampler sampler);
    void DebugListMaterialTextures(aiMaterial* material);
    std::filesystem::path GetTexturePath(const std::filesystem::path& modelPath);
}
