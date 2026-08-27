#include "asset/ImporterHelper.h"
#include "asset/CPUImage.h"
#include <cstddef>
#include <cstdint>
#include <sys/types.h>
#include <vulkan/vulkan.hpp>

#include "asset/Importer.h"
#include "allocation/MaterialRegistry.h"
#include "rendering/CommandBuffer.h"
#include "allocation/BindingManager.h"
#include "rendering/GPUImage.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/GraphicsImage.h"
#include "rendering/ImageHelper.h"
#include "allocation/RessourceUsage.h"

void ImporterHelper::UploadTexture(std::span<const ImporterHelper::UploadCommand> commands, PoolStageBuffer& buffer, vk::Queue queue, vk::CommandPool pool)
{
    CommandBuffer cmdBuffer(pool, queue);
    cmdBuffer.BeginSingleTime();
    {
        for (uint32_t i = 0; i < commands.size(); i++)
        {
            const ImporterHelper::UploadCommand& command = commands[i];
            ImageHelper::TransitionLayoutCommand(cmdBuffer, command.Image, 0, command.MipCount, 0, 1, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
            cmdBuffer.CopyBufferToImage(buffer.Internal(), command.Image.Image, vk::ImageLayout::eTransferDstOptimal, {command.CopyRegion, command.MipCount});
            ImageHelper::TransitionLayoutCommand(cmdBuffer, command.Image, 0, command.MipCount, 0, 1, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
        }
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
}

BindingManager::BindlessTexture ImporterHelper::LoadTexture(BindingManager& allocator, const std::vector<TextureImport>& textures, const std::vector<std::byte>& textureDatas, uint32_t textureIndex, MaterialRegistry& materialRegistry, PoolStageBuffer & uploadBuffer, ImporterHelper::UploadCommand* commands, uint32_t& commandCount, vk::Sampler sampler, vk::CommandPool uploadPool, vk::Queue uploadQueue)
{
    //DebugLayer::Log(DebugLayer::LogType::INFO, "Loading texture " + std::to_string(textureIndex) + " of type " + std::to_string(textures[textureIndex].Type) + " with " + std::to_string(textures[textureIndex].MipCount) + " mips, with size " + std::to_string(textures[textureIndex].Size) + " width, height = " + std::to_string(textures[textureIndex].Width) + ", " + std::to_string(textures[textureIndex].Height) + " and format " + std::to_string(textures[textureIndex].Format));
    const TextureImport & texture = textures[textureIndex];
    auto gpuTextureHandle = BindlessTexture::Register(allocator, RessourceUsage::Static, sampler, texture.Width, texture.Height,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        texture.Format,
        texture.MipCount);

    const auto& gpuTexture = gpuTextureHandle.Texture(allocator);

    UploadCommand uploadCommand
    {
        .MipCount = texture.MipCount,
        .Image = gpuTexture.Internal(),
    };

    uint32_t offset = 0;
    for (uint32_t i = 0; i < texture.MipCount; i++)
    {
        auto& mip = textures[textureIndex + i];
        offset += mip.Size;
    }

    if(!uploadBuffer.CanAllocate(offset) || commandCount >= 16)
    {
        UploadTexture({commands, commandCount}, uploadBuffer, uploadQueue, uploadPool);
        uploadBuffer.Clear();
        commandCount = 0;
    }

    uint32_t bufferOffset = uploadBuffer.Offset();

    for (uint32_t i = 0; i < texture.MipCount; i++)
    {
        auto& mip = textures[textureIndex + i];
        uploadCommand.CopyRegion[i] = vk::BufferImageCopy{}
            .setBufferOffset(bufferOffset)
            .setImageSubresource(vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, i, 0, 1})
            .setImageOffset(vk::Offset3D{0, 0, 0})
            .setImageExtent(vk::Extent3D{static_cast<uint32_t>(mip.Width), static_cast<uint32_t>(mip.Height), 1});
        bufferOffset += mip.Size;
    }
    uploadBuffer.CopyToBuffer(textureDatas.data() + texture.Offset, offset);
    commands[commandCount++] = uploadCommand;

    return gpuTextureHandle;
}

GPUImage ImporterHelper::ImageFromCPU(const CPUImage& cpuImg, vk::CommandPool pool, vk::Queue queue)
{
    GPUImage gpuImg(cpuImg.Width(),
        cpuImg.Height(),
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        cpuImg.GetGPUFormat());
    gpuImg.UploadAndGenerateMip(cpuImg.Data(), cpuImg.Width(), cpuImg.Height(), cpuImg.Channels(), pool, queue);

    return gpuImg;
}

GraphicsImage ImporterHelper::CubemapFromCPU(BindingManager& allocator, uint32_t dstBinding, vk::CommandPool pool, vk::Queue queue, vk::Sampler sampler, const CPUImage& front, const CPUImage& back, const CPUImage& left, const CPUImage& right, const CPUImage& top, const CPUImage& bottom)
{
    assert(front.Width() == back.Width() && front.Width() == left.Width() && front.Width() == right.Width() && front.Width() == top.Width() && front.Width() == bottom.Width());
    assert(front.Height() == back.Height() && front.Height() == left.Height() && front.Height() == right.Height() && front.Height() == top.Height() && front.Height() == bottom.Height());
    assert(front.Channels() == back.Channels() && front.Channels() == left.Channels() && front.Channels() == right.Channels() && front.Channels() == top.Channels() && front.Channels() == bottom.Channels());

    GraphicsImage gpuCubeMap(allocator, dstBinding, sampler,RessourceUsage::Static,
        front.Width(), front.Height(),
        6, vk::ImageCreateFlagBits::eCubeCompatible,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        front.GetGPUFormat(), ImageHelper::GetMipCount(front.Width(), front.Height()), vk::ImageTiling::eOptimal, vk::SampleCountFlagBits::e1);

    const CPUImage* stage[6] = { &right, &left, &top, &bottom, &front, &back };
    vk::BufferImageCopy copyRegions[6];
    std::span<vk::BufferImageCopy> regions(copyRegions, 6);
    vk::BufferImageCopy base = vk::BufferImageCopy{}
        .setImageSubresource(vk::ImageSubresourceLayers{}.setAspectMask(vk::ImageAspectFlagBits::eColor).setLayerCount(1))
        .setImageExtent(vk::Extent3D{static_cast<uint32_t>(front.Width()), static_cast<uint32_t>(front.Height()), 1});

    vk::DeviceSize size = front.Width() * front.Height() * front.Channels();

    StageBuffer stageBuffer(size * 6, StageBuffer::Usage::Upload);

    vk::DeviceSize offset = 0;
    for (uint32_t i = 0; i < 6; i++)
    {
        const CPUImage* img = stage[i];
        stageBuffer.MapAndCopyToBuffer(img->Data(), offset, size);

        copyRegions[i] = base;
        copyRegions[i].bufferOffset = offset;
        copyRegions[i].imageSubresource.baseArrayLayer = i;
        offset += size;
    }

    const auto & image = gpuCubeMap.Internal();

    CommandBuffer cmdBuffer(pool, queue);
    cmdBuffer.BeginSingleTime();
    {
        ImageHelper::TransitionLayoutCommand(cmdBuffer, image, 0, 1, 0, 6, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        cmdBuffer.CopyBufferToImage(stageBuffer.Internal(), image.Image, vk::ImageLayout::eTransferDstOptimal, regions);
        ImageHelper::GenerateMipmapsCommand(cmdBuffer, gpuCubeMap.Internal());
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();

    return gpuCubeMap;
}
