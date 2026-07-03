#pragma once

#include "asset/CPUImage.h"
#include "asset/Importer.h"
#include "allocation/DescriptorAllocator.h"
#include "rendering/GPUImage.h"
#include "allocation/MaterialRegistry.h"
#include "rendering/GraphicsImage.h"
#include <cstdint>

namespace ImporterHelper
{
    struct UploadCommand
    {
        uint32_t StageOffset;
        uint32_t StageSize;
        uint32_t MipCount;
        ImageReference Image;
        VkBufferImageCopy CopyRegion[16];
    };

    void UploadTexture(std::span<const ImporterHelper::UploadCommand> commands, PoolStageBuffer& buffer, VkQueue queue, VkCommandPool pool);
    DescriptorAllocator::BindlessTexture LoadTexture(DescriptorAllocator& allocator, const std::vector<TextureImport>& textures, const std::vector<std::byte>& texturesData, uint32_t textureIndex, MaterialRegistry& materialRegistry, PoolStageBuffer & uploadBuffer, ImporterHelper::UploadCommand* commands, uint32_t& commandCount, VkSampler sampler, VkCommandPool uploadPool, VkQueue uploadQueue);
    GPUImage ImageFromCPU(const CPUImage& cpuImg, VkCommandPool pool, VkQueue queue);
    GraphicsImage CubemapFromCPU(DescriptorAllocator& allocator, uint32_t dstBinding, VkCommandPool pool, VkQueue queue, VkSampler sampler, const CPUImage& front, const CPUImage& back, const CPUImage& left, const CPUImage& right, const CPUImage& top, const CPUImage& bottom);
}
