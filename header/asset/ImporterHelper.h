#pragma once

#include "asset/CPUImage.h"
#include "asset/Importer.h"
#include "allocation/BindingManager.h"
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
        vk::BufferImageCopy CopyRegion[16];
    };

    void UploadTexture(std::span<const ImporterHelper::UploadCommand> commands, PoolStageBuffer& buffer, vk::Queue queue, vk::CommandPool pool);
    BindingManager::BindlessTexture LoadTexture(BindingManager& allocator, const std::vector<TextureImport>& textures, const std::vector<std::byte>& texturesData, uint32_t textureIndex, MaterialRegistry& materialRegistry, PoolStageBuffer & uploadBuffer, ImporterHelper::UploadCommand* commands, uint32_t& commandCount, vk::Sampler sampler, vk::CommandPool uploadPool, vk::Queue uploadQueue);
    GPUImage ImageFromCPU(const CPUImage& cpuImg, vk::CommandPool pool, vk::Queue queue);
    GraphicsImage CubemapFromCPU(BindingManager& allocator, uint32_t dstBinding, vk::CommandPool pool, vk::Queue queue, vk::Sampler sampler, const CPUImage& front, const CPUImage& back, const CPUImage& left, const CPUImage& right, const CPUImage& top, const CPUImage& bottom);
}
