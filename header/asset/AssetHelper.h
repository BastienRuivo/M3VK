#pragma once

#include "asset/AssetImporter.h"
#include "asset/CPUImage.h"
#include "rendering/DescriptorAllocator.h"
#include "rendering/GPUImage.h"
#include "registry/MaterialRegistry.h"
#include "registry/MeshRegistry.h"
#include "rendering/GraphicsImage.h"
#include <cstdint>

namespace AssetHelper
{
    struct UploadCommand
    {
        uint32_t StageOffset;
        uint32_t StageSize;
        uint32_t MipCount;
        ImageReference Image;
        VkBufferImageCopy CopyRegion[16];
    };

    void Load3DModel(DescriptorAllocator& allocator, const std::string & modelPath, MeshRegistry & meshRegistry, MaterialRegistry & materialRegistry, VkCommandPool uploadPool, VkQueue uploadQueue, VkSampler sampler);
    uint32_t LoadTexture(DescriptorAllocator& allocator, AssetImporter& importer, uint32_t textureIndex, MaterialRegistry& materialRegistry, PoolStageBuffer & uploadBuffer, AssetHelper::UploadCommand* commands, uint32_t& commandCount, VkSampler sampler, VkCommandPool uploadPool, VkQueue uploadQueue);
    GPUImage ImageFromCPU(const CPUImage& cpuImg, VkCommandPool pool, VkQueue queue);
    GraphicsImage CubemapFromCPU(DescriptorAllocator& allocator, uint32_t dstBinding, VkCommandPool pool, VkQueue queue, VkSampler sampler, const CPUImage& front, const CPUImage& back, const CPUImage& left, const CPUImage& right, const CPUImage& top, const CPUImage& bottom);
}
