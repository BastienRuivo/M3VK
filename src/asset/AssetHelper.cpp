#include "asset/AssetHelper.h"
#include "application/ApplicationHelper.h"
#include "application/DebugLayer.h"
#include "asset/AssetImporter.h"
#include "asset/CPUImage.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>

#include "glm/ext/vector_float3.hpp"
#include "registry/MaterialRegistry.h"
#include "rendering/CommandBuffer.h"
#include "rendering/DescriptorAllocator.h"
#include "rendering/GPUImage.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/ImageHelper.h"

void Upload(AssetHelper::UploadCommand* commands, uint32_t commandCount, PoolStageBuffer& buffer, VkQueue queue, VkCommandPool pool)
{
    CommandBuffer cmdBuffer(pool, queue);
    cmdBuffer.BeginSingleTime();
    {
        for (uint32_t i = 0; i < commandCount; i++)
        {
            AssetHelper::UploadCommand& command = commands[i];
            ImageHelper::TransitionLayoutCommand(cmdBuffer, command.Image, 0, command.MipCount, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            cmdBuffer.CopyBufferToImage(buffer.Internal(), command.Image.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, command.CopyRegion, command.MipCount);
            ImageHelper::TransitionLayoutCommand(cmdBuffer, command.Image, 0, command.MipCount, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
}

uint32_t AssetHelper::LoadTexture(DescriptorAllocator& allocator, AssetImporter& importer, uint32_t textureIndex, MaterialRegistry& materialRegistry, PoolStageBuffer & uploadBuffer, AssetHelper::UploadCommand* commands, uint32_t& commandCount, VkSampler sampler, VkCommandPool uploadPool, VkQueue uploadQueue)
{
    //DebugLayer::Log(DebugLayer::LogType::INFO, "Loading texture " + std::to_string(textureIndex) + " of type " + std::to_string(importer.Textures[textureIndex].Type) + " with " + std::to_string(importer.Textures[textureIndex].MipCount) + " mips, with size " + std::to_string(importer.Textures[textureIndex].Size) + " width, height = " + std::to_string(importer.Textures[textureIndex].Width) + ", " + std::to_string(importer.Textures[textureIndex].Height) + " and format " + std::to_string(importer.Textures[textureIndex].Format));
    TextureImport & texture = importer.Textures[textureIndex];
    GPUAllocatedImage gpuTexture(texture.Width, texture.Height, texture.MipCount, texture.Format,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT |VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    UploadCommand uploadCommand
    {
        .MipCount = texture.MipCount,
        .Image = gpuTexture.Internal(),
    };

    uint32_t offset = 0;
    for (uint32_t i = 0; i < texture.MipCount; i++)
    {
        auto& mip = importer.Textures[textureIndex + i];
        offset += mip.Size;
    }

    if(!uploadBuffer.CanAllocate(offset) || commandCount >= 16)
    {
        Upload(commands, commandCount, uploadBuffer, uploadQueue, uploadPool);
        uploadBuffer.Clear();
        commandCount = 0;
    }

    uint32_t bufferOffset = uploadBuffer.Offset();

    for (uint32_t i = 0; i < texture.MipCount; i++)
    {
        auto& mip = importer.Textures[textureIndex + i];
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
    uploadBuffer.CopyToBuffer(importer.TextureDatas.data() + texture.Offset, offset);
    commands[commandCount++] = uploadCommand;

    return materialRegistry.RegisterTexture(allocator, std::move(gpuTexture), sampler);
}

void AssetHelper::Load3DModel(DescriptorAllocator& allocator, const std::string & modelPath, MeshRegistry & meshRegistry, MaterialRegistry & materialRegistry, VkCommandPool uploadPool, VkQueue uploadQueue, VkSampler sampler)
{
    AssetImporter importer;

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    if(!AssetImporter::Load(importer, modelPath))
    {
        return ;
    }


    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

    DebugLayer::Log(DebugLayer::LogType::INFO, "AssetImporter load time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()) + "ms");

    uint32_t materialOffset = materialRegistry.MaterialsCount();
    {
        PoolStageBuffer uploadBuffer(4096 * 4096 * 4, StageBuffer::Usage::Upload);
        uploadBuffer.Map();
        std::array<AssetHelper::UploadCommand, 16> uploadCommands;
        uint32_t uploadCommandCount = 0;

        MaterialProperties defaultMaterial = materialRegistry.DefaultMaterial();

        uint32_t materialOffset = materialRegistry.MaterialsCount();
        for(unsigned int i = 0; i < importer.Header.MaterialCount; i++)
        {
            const auto & material = importer.Materials[i];
            MaterialProperties gpuMaterial = material;

            gpuMaterial.BaseColorTexId = material.BaseColorTexId == UINT32_MAX ? defaultMaterial.BaseColorTexId: LoadTexture(allocator, importer, material.BaseColorTexId, materialRegistry, uploadBuffer, uploadCommands.data(), uploadCommandCount, sampler, uploadPool, uploadQueue);
            gpuMaterial.NormalMapTexId = material.NormalMapTexId == UINT32_MAX ? defaultMaterial.NormalMapTexId : LoadTexture(allocator, importer, material.NormalMapTexId, materialRegistry, uploadBuffer, uploadCommands.data(), uploadCommandCount, sampler, uploadPool, uploadQueue);
            gpuMaterial.MRAOTexId = material.MRAOTexId == UINT32_MAX ? defaultMaterial.MRAOTexId : LoadTexture(allocator, importer, material.MRAOTexId, materialRegistry, uploadBuffer, uploadCommands.data(), uploadCommandCount, sampler, uploadPool, uploadQueue);

            materialRegistry.RegisterMaterial(gpuMaterial);
        }

        Upload(uploadCommands.data(), uploadCommandCount, uploadBuffer, uploadQueue, uploadPool);
        uploadBuffer.Unmap();
    }

    std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();

    DebugLayer::Log(DebugLayer::LogType::INFO, "AssetImporter material load time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count()) + "ms");

    std::vector<MeshHandle> subMeshes;

    uint32_t meshCount = importer.Header.SubMeshCount;
    for(unsigned int i = 0; i < meshCount; i++)
    {
        const auto & mesh = importer.SubMeshes[i];

        std::span<const Vertex> vertices(importer.VertexDatas.data() + mesh.VertexOffset, mesh.VertexCount);
        std::span<const uint32_t> indices(importer.IndexDatas.data() + mesh.IndexOffset, mesh.IndexCount);

        uint32_t submesh = meshRegistry.RegisterMesh(vertices, indices);
        InstanceData instance = {
            .LocalToWorldMatrix = ApplicationHelper::TranslateRotateScale(glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(1.0f, 1.0f, 1.0f)),
            .AabbMin = mesh.AABBMin,
            .MaterialIndex = materialOffset + mesh.MaterialIndex,
            .AabbMax = mesh.AABBMax,
            .MeshIndex = submesh
        };

        meshRegistry.RegisterInstance(instance);
    }

    std::chrono::high_resolution_clock::time_point t4 = std::chrono::high_resolution_clock::now();
    DebugLayer::Log(DebugLayer::LogType::INFO, "AssetImporter mesh load time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count()) + "ms");
    DebugLayer::Log(DebugLayer::LogType::INFO, "AssetImporter load time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t1).count()) + "ms -------------------------------------------");
}

GPUAllocatedImage AssetHelper::ImageFromCPU(const CPUImage& cpuImg, VkCommandPool pool, VkQueue queue)
{
    GPUAllocatedImage gpuImg(cpuImg.Width(),
        cpuImg.Height(),
        cpuImg.GetGPUFormat(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    gpuImg.UploadAndGenerateMip(cpuImg.Data(), cpuImg.Width(), cpuImg.Height(), cpuImg.Channels(), pool, queue);

    return gpuImg;
}
