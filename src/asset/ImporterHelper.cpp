#include "asset/ImporterHelper.h"
#include "asset/CPUImage.h"
#include <cstddef>
#include <cstdint>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>

#include "asset/Importer.h"
#include "registry/MaterialRegistry.h"
#include "rendering/CommandBuffer.h"
#include "rendering/DescriptorAllocator.h"
#include "rendering/GPUImage.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/GraphicsImage.h"
#include "rendering/ImageHelper.h"
#include "rendering/RessourceUsage.h"

void ImporterHelper::UploadTexture(ImporterHelper::UploadCommand* commands, uint32_t commandCount, PoolStageBuffer& buffer, VkQueue queue, VkCommandPool pool)
{
    CommandBuffer cmdBuffer(pool, queue);
    cmdBuffer.BeginSingleTime();
    {
        for (uint32_t i = 0; i < commandCount; i++)
        {
            ImporterHelper::UploadCommand& command = commands[i];
            ImageHelper::TransitionLayoutCommand(cmdBuffer, command.Image, 0, command.MipCount, 0, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            cmdBuffer.CopyBufferToImage(buffer.Internal(), command.Image.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, command.CopyRegion, command.MipCount);
            ImageHelper::TransitionLayoutCommand(cmdBuffer, command.Image, 0, command.MipCount, 0, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
}

uint32_t ImporterHelper::LoadTexture(DescriptorAllocator& allocator, const std::vector<TextureImport>& textures, const std::vector<std::byte>& textureDatas, uint32_t textureIndex, MaterialRegistry& materialRegistry, PoolStageBuffer & uploadBuffer, ImporterHelper::UploadCommand* commands, uint32_t& commandCount, VkSampler sampler, VkCommandPool uploadPool, VkQueue uploadQueue)
{
    //DebugLayer::Log(DebugLayer::LogType::INFO, "Loading texture " + std::to_string(textureIndex) + " of type " + std::to_string(textures[textureIndex].Type) + " with " + std::to_string(textures[textureIndex].MipCount) + " mips, with size " + std::to_string(textures[textureIndex].Size) + " width, height = " + std::to_string(textures[textureIndex].Width) + ", " + std::to_string(textures[textureIndex].Height) + " and format " + std::to_string(textures[textureIndex].Format));
    const TextureImport & texture = textures[textureIndex];
    GPUImage gpuTexture(texture.Width, texture.Height,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        texture.Format,
        texture.MipCount);

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
        UploadTexture(commands, commandCount, uploadBuffer, uploadQueue, uploadPool);
        uploadBuffer.Clear();
        commandCount = 0;
    }

    uint32_t bufferOffset = uploadBuffer.Offset();

    for (uint32_t i = 0; i < texture.MipCount; i++)
    {
        auto& mip = textures[textureIndex + i];
        uploadCommand.CopyRegion[i] =
        {
            .bufferOffset = bufferOffset,
            .imageSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .imageOffset =
            {
                .x = 0,
                .y = 0,
                .z = 0
            },
            .imageExtent =
            {
                .width = static_cast<uint32_t>(mip.Width),
                .height = static_cast<uint32_t>(mip.Height),
                .depth = 1
            }
        };
        bufferOffset += mip.Size;
    }
    uploadBuffer.CopyToBuffer(textureDatas.data() + texture.Offset, offset);
    commands[commandCount++] = uploadCommand;

    return materialRegistry.RegisterTexture(allocator, std::move(gpuTexture), sampler);
}

GPUImage ImporterHelper::ImageFromCPU(const CPUImage& cpuImg, VkCommandPool pool, VkQueue queue)
{
    GPUImage gpuImg(cpuImg.Width(),
        cpuImg.Height(),
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        cpuImg.GetGPUFormat());
    gpuImg.UploadAndGenerateMip(cpuImg.Data(), cpuImg.Width(), cpuImg.Height(), cpuImg.Channels(), pool, queue);

    return gpuImg;
}

GraphicsImage ImporterHelper::CubemapFromCPU(DescriptorAllocator& allocator, uint32_t dstBinding, VkCommandPool pool, VkQueue queue, VkSampler sampler, const CPUImage& front, const CPUImage& back, const CPUImage& left, const CPUImage& right, const CPUImage& top, const CPUImage& bottom)
{
    assert(front.Width() == back.Width() && front.Width() == left.Width() && front.Width() == right.Width() && front.Width() == top.Width() && front.Width() == bottom.Width());
    assert(front.Height() == back.Height() && front.Height() == left.Height() && front.Height() == right.Height() && front.Height() == top.Height() && front.Height() == bottom.Height());
    assert(front.Channels() == back.Channels() && front.Channels() == left.Channels() && front.Channels() == right.Channels() && front.Channels() == top.Channels() && front.Channels() == bottom.Channels());

    GraphicsImage gpuCubeMap(allocator, dstBinding, sampler,RessourceUsage::Static,
        front.Width(), front.Height(),
        6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        front.GetGPUFormat(), ImageHelper::GetMipCount(front.Width(), front.Height()), VK_IMAGE_TILING_OPTIMAL, VK_SAMPLE_COUNT_1_BIT);

    const CPUImage* stage[6] = { &right, &left, &top, &bottom, &front, &back };
    VkBufferImageCopy copyRegions[6];
    VkBufferImageCopy base = {};
    base.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    base.imageSubresource.layerCount = 1;
    base.imageExtent = { static_cast<uint32_t>(front.Width()), static_cast<uint32_t>(front.Height()), 1 };

    VkDeviceSize size = front.Width() * front.Height() * front.Channels();

    StageBuffer stageBuffer(size * 6, StageBuffer::Usage::Upload);

    VkDeviceSize offset = 0;
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
        ImageHelper::TransitionLayoutCommand(cmdBuffer, image, 0, 1, 0, 6, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        cmdBuffer.CopyBufferToImage(stageBuffer.Internal(), image.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copyRegions, 6);
        ImageHelper::GenerateMipmapsCommand(cmdBuffer, gpuCubeMap.Internal());
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();

    return gpuCubeMap;
}
