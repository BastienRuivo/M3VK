#pragma once

#include "assimp/material.h"
#include "rendering/DescriptorPool.h"
#include "rendering/GPUImage.h"
#include "registry/MaterialRegistry.h"
#include "registry/MeshRegistry.h"
#include "rendering/Renderer.h"
#include <filesystem>

namespace AssetHelper
{
    aiTextureType SelectTextureType(std::span<const aiTextureType> types, const aiMaterial* material, uint32_t& textureCount);
    ImageHelper::ImageBinding LoadTexture(const aiMaterial* material, std::vector<GPUAllocatedImage> & textures, const std::filesystem::path rootPath, std::span<const aiTextureType> types, bool& hasFoundTexture, const ImageHelper::ImageBinding & fallback, VkSampler sampler, VkCommandPool uploadPool, VkQueue uploadQueue);
    Renderer Load3DModel(const std::string & modelPath, MeshRegistry & meshRegistry, MaterialRegistry & materialRegistry, std::vector<GPUAllocatedImage> & textures, std::vector<Material> & materials, int defaultMaterial, DescriptorPool& descriptorPool, VkCommandPool cmdPool, VkQueue queue, VkSampler sampler);
}
