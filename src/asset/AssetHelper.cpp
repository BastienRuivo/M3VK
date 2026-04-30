#include "asset/AssetHelper.h"
#include "application/DebugLayer.h"
#include "asset/AssetImporter.h"
#include "asset/CPUImage.h"
#include <array>
#include <cstdint>
#include <span>
#include <filesystem>
#include <string>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>

#include "glm/ext/vector_float3.hpp"
#include "rendering/BufferHelper.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GPUImage.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/ImageHelper.h"

void Upload(AssetHelper::UploadCommand* commands, uint32_t commandCount, PoolStageBuffer& buffer, std::vector<GPUAllocatedImage> & textures, VkQueue queue, VkCommandPool pool)
{
    CommandBuffer cmdBuffer(pool, queue);
    cmdBuffer.BeginSingleTime();
    {
        for (uint32_t i = 0; i < commandCount; i++)
        {
            auto& command = commands[i];
            auto& texture = textures[command.TextureIndex];
            ImageHelper::TransitionLayoutCommand(cmdBuffer, texture.Internal(), 0, command.MipCount, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            cmdBuffer.CopyBufferToImage(buffer.Internal(), texture.Internal().Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, command.CopyRegion, command.MipCount);
            ImageHelper::TransitionLayoutCommand(cmdBuffer, texture.Internal(), 0, command.MipCount, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
}

ImageHelper::ImageBinding AssetHelper::LoadTexture(AssetImporter& importer, PoolStageBuffer & uploadBuffer, AssetHelper::UploadCommand* commands, uint32_t& commandCount, uint32_t textureIndex, std::vector<GPUAllocatedImage> & textures, VkSampler sampler, VkCommandPool uploadPool, VkQueue uploadQueue)
{
    //DebugLayer::Log(DebugLayer::LogType::INFO, "Loading texture " + std::to_string(textureIndex) + " of type " + std::to_string(importer.Textures[textureIndex].Type) + " with " + std::to_string(importer.Textures[textureIndex].MipCount) + " mips, with size " + std::to_string(importer.Textures[textureIndex].Size) + " width, height = " + std::to_string(importer.Textures[textureIndex].Width) + ", " + std::to_string(importer.Textures[textureIndex].Height) + " and format " + std::to_string(importer.Textures[textureIndex].Format));
    auto & texture = importer.Textures[textureIndex];
    GPUAllocatedImage gpuTexture(texture.Width, texture.Height, texture.MipCount, texture.Format,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT |VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    UploadCommand uploadCommand
    {
        .MipCount = texture.MipCount
    };

    uint32_t offset = 0;
    for (uint32_t i = 0; i < texture.MipCount; i++)
    {
        auto& mip = importer.Textures[textureIndex + i];
        offset += mip.Size;
    }

    if(!uploadBuffer.CanAllocate(offset))
    {
        Upload(commands, commandCount, uploadBuffer, textures, uploadQueue, uploadPool);
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

    ImageHelper::ImageBinding binding = ImageHelper::ImageBinding(gpuTexture.Internal(), sampler);
    textures.push_back(std::move(gpuTexture));
    uploadCommand.TextureIndex = textures.size() - 1;

    commands[commandCount++] = uploadCommand;

    return binding;
}

Renderer AssetHelper::Load3DModel(const std::string & modelPath, MeshRegistry & meshRegistry, MaterialRegistry & materialRegistry, std::vector<GPUAllocatedImage> & textures, std::vector<Material> & materials, int defaultMaterial, DescriptorPool& descriptorPool, VkCommandPool uploadPool, VkQueue uploadQueue, VkSampler sampler)
{
    AssetImporter importer;

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    AssetImporter::Load(importer, modelPath);

    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

    uint32_t materialOffset = materials.size();
    DebugLayer::Log(DebugLayer::LogType::INFO, "AssetImporter load time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()) + "ms");

    {
        PoolStageBuffer uploadBuffer(1024 * 1024 * 16, StageBuffer::Usage::Upload);
        uploadBuffer.Map();
        std::array<AssetHelper::UploadCommand, 16> uploadCommands;
        uint32_t uploadCommandCount = 0;

        int materialOffset = materials.size();
        for(unsigned int i = 0; i < importer.Header.MaterialCount; i++)
        {
            const auto & material = importer.Materials[i];
            MaterialProperties gpuMaterial = material.MatProperties;

            BufferHelper::BufferBinding materialBinding = materialRegistry.Register(gpuMaterial);
            ImageHelper::ImageBinding baseColorBinding = material.BaseColorTexId == UINT32_MAX ? materials[defaultMaterial].BaseColorTex : LoadTexture(importer, uploadBuffer, uploadCommands.data(), uploadCommandCount, material.BaseColorTexId, textures, sampler, uploadPool, uploadQueue);
            ImageHelper::ImageBinding normalBinding = material.NormalMapTexId == UINT32_MAX ? materials[defaultMaterial].NormalMapTex : LoadTexture(importer, uploadBuffer, uploadCommands.data(), uploadCommandCount, material.NormalMapTexId, textures, sampler, uploadPool, uploadQueue);
            ImageHelper::ImageBinding mraoBinding = material.MRAOTexId == UINT32_MAX ? materials[defaultMaterial].MRAOTex : LoadTexture(importer, uploadBuffer, uploadCommands.data(), uploadCommandCount, material.MRAOTexId, textures, sampler, uploadPool, uploadQueue);

            materials.emplace_back(baseColorBinding, normalBinding, mraoBinding, materialBinding, descriptorPool);
        }

        Upload(uploadCommands.data(), uploadCommandCount, uploadBuffer, textures, uploadQueue, uploadPool);
        uploadBuffer.Unmap();
    }

    std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();

    DebugLayer::Log(DebugLayer::LogType::INFO, "AssetImporter material load time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count()) + "ms");

    std::vector<SubMesh> subMeshes;
    Renderer renderer(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));

    uint32_t meshCount = importer.Header.SubMeshCount;
    for(unsigned int i = 0; i < meshCount; i++)
    {
        const auto & mesh = importer.SubMeshes[i];

        std::span<const Vertex> vertices(importer.VertexDatas.data() + mesh.VertexOffset, mesh.VertexCount);
        std::span<const uint32_t> indices(importer.IndexDatas.data() + mesh.IndexOffset, mesh.IndexCount);

        SubMesh submesh = meshRegistry.Register(vertices, indices);
        subMeshes.push_back(submesh);

        renderer.AddMesh(submesh, materials[materialOffset + mesh.MaterialIndex]);
    }

    std::chrono::high_resolution_clock::time_point t4 = std::chrono::high_resolution_clock::now();
    DebugLayer::Log(DebugLayer::LogType::INFO, "AssetImporter mesh load time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count()) + "ms");
    DebugLayer::Log(DebugLayer::LogType::INFO, "AssetImporter load time: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t1).count()) + "ms -------------------------------------------");

    return renderer;
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
