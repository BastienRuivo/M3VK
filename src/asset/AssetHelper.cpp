#include "asset/AssetHelper.h"
#include "application/DebugLayer.h"
#include "asset/AssetExporter.h"
#include "assimp/material.h"
#include <cstdint>
#include <span>
#include <filesystem>
#include <string>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>

#include "assimp/types.h"
#include "glm/ext/vector_float3.hpp"
#include "rendering/BufferHelper.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GPUImage.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/ImageHelper.h"

aiTextureType AssetHelper::SelectTextureType(std::span<const aiTextureType> types, const aiMaterial* material, uint32_t& textureCount)
{
    for(const auto& type : types)
    {
        int count = material->GetTextureCount(type);
        if(count > 0)
        {
            textureCount = count;
            return type;
        }
    }
    return aiTextureType::aiTextureType_NONE;
}

std::filesystem::path AssetHelper::GetTexturePath(const std::filesystem::path& modelPath)
{
    int pathIndex = 0;
    const int modelRoot = 1;
    std::filesystem::path texturePath;

    for(const auto& path : modelPath)
    {
        texturePath /= path;
        pathIndex++;
        if(pathIndex == modelRoot + 1)
        {
            break;
        }
    }

    return texturePath / "textures";
}

ImageHelper::ImageBinding AssetHelper::LoadTexture(AssetExporter& exporter, uint32_t textureIndex, std::vector<GPUAllocatedImage> & textures, VkSampler sampler, VkCommandPool uploadPool, VkQueue uploadQueue)
{
    auto & texture = exporter.Textures[textureIndex];
    GPUAllocatedImage gpuTexture(texture.Width, texture.Height, texture.MipCount, texture.Format,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT |VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkBufferImageCopy copyRegion[16];

    uint32_t offset = 0;
    for (uint32_t i = 0; i < texture.MipCount; i++)
    {
        auto& mip = exporter.Textures[textureIndex + i];
        copyRegion[i] =
        {
            .bufferOffset = offset,
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
        offset += mip.Size;
    }

    StageBuffer stagingBuffer(offset, StageBuffer::Usage::Upload);
    stagingBuffer.MapAndCopyToBuffer(exporter.TextureDatas.data() + texture.Offset, offset);

    CommandBuffer cmdBuffer(uploadPool, uploadQueue);
    cmdBuffer.BeginSingleTime();
    {

        ImageHelper::TransitionLayoutCommand(cmdBuffer, gpuTexture.Internal(), 0, texture.MipCount, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        cmdBuffer.CopyBufferToImage(stagingBuffer.Internal(), gpuTexture.Internal().Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copyRegion, texture.MipCount);
        ImageHelper::TransitionLayoutCommand(cmdBuffer, gpuTexture.Internal(), 0, texture.MipCount, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();

    ImageHelper::ImageBinding binding = ImageHelper::ImageBinding(gpuTexture.Internal(), sampler);
    textures.push_back(std::move(gpuTexture));
    return binding;
}

void AssetHelper::DebugListMaterialTextures(aiMaterial* material) {
    // Iterate through all possible Assimp texture types
    for (unsigned int type = aiTextureType_NONE; type < AI_TEXTURE_TYPE_MAX; ++type)
    {
        aiTextureType textureType = static_cast<aiTextureType>(type);
        unsigned int count = material->GetTextureCount(textureType);

        for (unsigned int i = 0; i < count; ++i)
        {
            aiString path;
            if (material->GetTexture(textureType, i, &path) == AI_SUCCESS)
            {
                DebugLayer::Log(DebugLayer::LogType::INFO, "[Texture Found] Type: " + std::to_string(type) + " | Index: " + std::to_string(i) + " | Path: " + path.C_Str());
            }
        }
    }
}

Renderer AssetHelper::Load3DModel(const std::string & modelPath, MeshRegistry & meshRegistry, MaterialRegistry & materialRegistry, std::vector<GPUAllocatedImage> & textures, std::vector<Material> & materials, int defaultMaterial, DescriptorPool& descriptorPool, VkCommandPool uploadPool, VkQueue uploadQueue, VkSampler sampler)
{
    AssetExporter exporter = AssetExporter::Load3DModel(modelPath, uploadPool, uploadQueue);
    AssetExporter::Write(exporter);

    int materialOffset = materials.size();
    std::filesystem::path textureRootPath = GetTexturePath(modelPath);

    for(unsigned int i = 0; i < exporter.Header.MaterialCount; i++)
    {
        const auto & material = exporter.Materials[i];
        GPUMaterial gpuMaterial = material.MaterialProperties;

        BufferHelper::BufferBinding materialBinding = materialRegistry.Register(gpuMaterial);
        ImageHelper::ImageBinding baseColorBinding = material.BaseColorTexId == UINT32_MAX ? materials[defaultMaterial].BaseColorTex : LoadTexture(exporter, material.BaseColorTexId, textures, sampler, uploadPool, uploadQueue);
        ImageHelper::ImageBinding normalBinding = material.NormalMapTexId == UINT32_MAX ? materials[defaultMaterial].NormalMapTex : LoadTexture(exporter, material.NormalMapTexId, textures, sampler, uploadPool, uploadQueue);
        ImageHelper::ImageBinding mraoBinding = material.MRAOTexId == UINT32_MAX ? materials[defaultMaterial].MRAOTex : LoadTexture(exporter, material.MRAOTexId, textures, sampler, uploadPool, uploadQueue);

        materials.emplace_back(baseColorBinding, normalBinding, mraoBinding, materialBinding, descriptorPool);
    }

    std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();

    std::vector<SubMesh> subMeshes;
    Renderer renderer(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));

    uint32_t meshCount = exporter.Header.SubMeshCount;
    for(unsigned int i = 0; i < meshCount; i++)
    {
        const auto & mesh = exporter.SubMeshes[i];

        std::span<const Vertex> vertices(exporter.VertexDatas.data() + mesh.VertexOffset, mesh.VertexCount);
        std::span<const uint32_t> indices(exporter.IndexDatas.data() + mesh.IndexOffset, mesh.IndexCount);

        SubMesh submesh = meshRegistry.Register(vertices, indices);
        subMeshes.push_back(submesh);

        renderer.AddMesh(submesh, materials[materialOffset + i]);
    }

    return renderer;
}
