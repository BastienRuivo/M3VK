#pragma once

#include "assimp/material.h"
#include "rendering/DescriptorPool.h"
#include "rendering/GPUImage.h"
#include "registry/MaterialRegistry.h"
#include "registry/MeshRegistry.h"
#include "rendering/Renderer.h"

namespace AssetHelper
{
    aiTextureType SelectTextureType(std::span<const aiTextureType> types, const aiMaterial* material, uint32_t& textureCount);
    Renderer Load3DModel(const std::string & modelPath, MeshRegistry & meshRegistry, MaterialRegistry & materialRegistry, std::vector<GPUAllocatedImage> & textures, std::vector<Material> & materials, int defaultMaterial, DescriptorPool& descriptorPool, VkCommandPool cmdPool, VkQueue queue, VkSampler sampler);
}
