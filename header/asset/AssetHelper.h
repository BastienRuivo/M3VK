#pragma once

#include "asset/AssetImporter.h"
#include "asset/CPUImage.h"
#include "rendering/DescriptorPool.h"
#include "rendering/GPUImage.h"
#include "registry/MaterialRegistry.h"
#include "registry/MeshRegistry.h"
#include "rendering/Renderer.h"
#include <cstdint>

namespace AssetHelper
{
    struct UploadCommand
    {
        uint32_t StageOffset;
        uint32_t StageSize;
        uint32_t TextureIndex;
        uint32_t MipCount;
        VkBufferImageCopy CopyRegion[16];
    };

    Renderer Load3DModel(const std::string & modelPath, MeshRegistry & meshRegistry, MaterialRegistry & materialRegistry, std::vector<GPUAllocatedImage> & textures, std::vector<Material> & materials, int defaultMaterial, DescriptorPool& descriptorPool, VkCommandPool uploadPool, VkQueue uploadQueue, VkSampler sampler);
    ImageHelper::ImageBinding LoadTexture(AssetImporter& importer, PoolStageBuffer & uploadBuffer, AssetHelper::UploadCommand* commands, uint32_t& commandCount, uint32_t textureIndex, std::vector<GPUAllocatedImage> & textures, VkSampler sampler, VkCommandPool uploadPool, VkQueue uploadQueue);
    GPUAllocatedImage ImageFromCPU(const CPUImage& cpuImg, VkCommandPool pool, VkQueue queue);
}
