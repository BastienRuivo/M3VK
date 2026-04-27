#include "asset/AssetHelper.h"
#include "application/DebugLayer.h"
#include "assimp/Importer.hpp"
#include "assimp/defs.h"
#include "assimp/material.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include <cstdint>
#include <span>
#include <filesystem>
#include <string>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>

#include "assimp/types.h"
#include "glm/ext/vector_float3.hpp"
#include "libs/tinyddsloader.h"
#include "rendering/GPUImage.h"
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

std::filesystem::path GetTexturePath(const std::filesystem::path& modelPath)
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

ImageHelper::ImageBinding AssetHelper::LoadTexture(const aiMaterial* material, std::vector<GPUAllocatedImage> & textures, const std::filesystem::path rootPath, std::span<const aiTextureType> types, bool& hasFoundTexture, const ImageHelper::ImageBinding & fallback, VkSampler sampler, VkCommandPool uploadPool, VkQueue uploadQueue)
{
    uint32_t textureCount = 0;
    aiTextureType textureType = SelectTextureType(types, material, textureCount);

    if(textureCount == 0) return fallback;


    aiString path;
    material->GetTexture(textureType, 0, &path);

    if(path.length == 0)
    {
        DebugLayer::Log(DebugLayer::LogType::WARNING, "Found a texture with a 0 length path");
        return fallback;
    }


    std::string rawPath = path.C_Str();
    std::replace(rawPath.begin(), rawPath.end(), '\\', '/');
    std::filesystem::path texturePath(rawPath);
    texturePath = texturePath.filename();
    texturePath = rootPath / texturePath;
    texturePath = texturePath.lexically_normal();


    if(!std::filesystem::exists(texturePath))
    {
        DebugLayer::Log(DebugLayer::LogType::WARNING, "Path does not exist " + texturePath.string());
        return fallback;
    }
    else if(texturePath.extension() == ".dds") // handle compressed textures directly
    {
        tinyddsloader::DDSFile dds;
        auto ret = dds.Load(texturePath.string().c_str());
        if(ret != tinyddsloader::Result::Success)
        {
            DebugLayer::Log(DebugLayer::LogType::WARNING, "Failed to load compressed texture " + texturePath.string());
            return fallback;
        }

        auto& texture = textures.emplace_back(dds, uploadPool, uploadQueue);
        hasFoundTexture = true;
        return ImageHelper::ImageBinding(texture.Internal(), sampler);
    }
    else
    {
        auto& texture = textures.emplace_back(CPUImage(texturePath, STBI_rgb_alpha), uploadPool, uploadQueue);
        hasFoundTexture = true;
        return ImageHelper::ImageBinding(texture.Internal(), sampler);
    }
}

void DebugListMaterialTextures(aiMaterial* material) {
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

Renderer AssetHelper::Load3DModel(const std::string & modelPath, MeshRegistry & meshRegistry, MaterialRegistry & materialRegistry, std::vector<GPUAllocatedImage> & textures, std::vector<Material> & materials, int defaultMaterial, DescriptorPool& descriptorPool, VkCommandPool cmdPool, VkQueue queue, VkSampler sampler)
{
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    DebugLayer::Log(DebugLayer::LogType::INFO, "Loading model: " + modelPath);

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(modelPath,
        aiProcess_Triangulate
        | aiProcess_JoinIdenticalVertices
        | aiProcess_SortByPType
        | aiProcess_GenUVCoords
        | aiProcess_FlipUVs
        | aiProcess_GlobalScale     // Handles FBX unit scaling (cm to m)
        | aiProcess_PreTransformVertices // Collapses the node hierarchy into the verticess
        | aiProcess_GenNormals
    );

    if(scene == nullptr)
    {
        DebugLayer::Log(DebugLayer::LogType::ERROR, importer.GetErrorString());
        DebugLayer::Log(DebugLayer::LogType::ERROR, "Can't open " + std::string(std::filesystem::current_path()) + "/" + modelPath);
        throw std::runtime_error(importer.GetErrorString());
    }

    int materialOffset = materials.size();
    std::filesystem::path textureRootPath = GetTexturePath(modelPath);

    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    DebugLayer::Log(DebugLayer::LogType::INFO, "Loaded model in " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(t2 - t1).count()) + "s");
    DebugLayer::Log(DebugLayer::LogType::INFO, "Found " + std::to_string(scene->mNumMaterials) + " materials in model");

    for(unsigned int i = 0; i < scene->mNumMaterials; i++)
    {
        aiMaterial* material = scene->mMaterials[i];



        // get base color value
        aiColor4D color;
        material->Get(AI_MATKEY_COLOR_DIFFUSE, color);

        ai_real metallic;
        material->Get(AI_MATKEY_METALLIC_FACTOR, metallic);

        ai_real roughness;
        material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);


        GPUMaterial gpuMaterial
        {
            .BaseColor = glm::vec4(static_cast<float>(color.r),
                static_cast<float>(color.g),
                static_cast<float>(color.b),
                static_cast<float>(color.a)),
            .Metallic = static_cast<float>(metallic),
            .Roughness = static_cast<float>(roughness)
        };

        bool hasProperties = gpuMaterial != GPUMaterial::Default();

#if M3VK_VERBOSE_LOG
        DebugListMaterialTextures(material);
#endif
        bool hasTexture = false;

        ImageHelper::ImageBinding baseColorTex = LoadTexture(material, textures, textureRootPath, {{ aiTextureType::aiTextureType_BASE_COLOR, aiTextureType::aiTextureType_DIFFUSE }}, hasTexture, materials[defaultMaterial].BaseColorTex, sampler, cmdPool, queue);
        ImageHelper::ImageBinding normalMapTex = LoadTexture(material, textures, textureRootPath, {{ aiTextureType::aiTextureType_NORMALS }}, hasTexture, materials[defaultMaterial].NormalMapTex, sampler, cmdPool, queue);
        ImageHelper::ImageBinding mraoTex = LoadTexture(material, textures, textureRootPath, {{ aiTextureType::aiTextureType_AMBIENT_OCCLUSION }}, hasTexture, materials[defaultMaterial].MRAOTex, sampler, cmdPool, queue);

        if(!hasTexture && !hasProperties)
        {
            DebugLayer::Log(DebugLayer::LogType::WARNING, "Fallback to default material for material at index " + std::to_string(i) + " of object " + modelPath);
            materials.emplace_back(materials[defaultMaterial]);
        }
        else
        {
            BufferHelper::BufferBinding binding = materialRegistry.Register(gpuMaterial);
            materials.emplace_back(baseColorTex, normalMapTex, mraoTex, binding, descriptorPool);
        }
    }

    std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();

    DebugLayer::Log(DebugLayer::LogType::INFO, "Loaded materials in " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(t3 - t2).count()) + "s");

    std::vector<SubMesh> subMeshes;
    Renderer renderer(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));

    uint32_t meshCount = scene->mNumMeshes;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    for(unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
        const aiMesh* mesh = scene->mMeshes[i];
        std::vector<Vertex> vertices(mesh->mNumVertices);
        std::vector<uint32_t> indices(mesh->mNumFaces * 3);

        vertexCount += mesh->mNumVertices;
        indexCount += mesh->mNumFaces * 3;

        bool hasTexcoords = mesh->HasTextureCoords(0);
        bool hasNormals = mesh->HasNormals();

        for(unsigned int j = 0; j < mesh->mNumVertices; j++)
        {
            vertices[j].pos = glm::vec3(mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z);

            if(hasNormals)
            {
                vertices[j].normal = glm::vec3(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z);
            }
            else
            {
                // TODO: Compute smooth normal in this case
                vertices[j].normal = glm::vec3(0, 1, 0);
            }
            vertices[j].texCoord = hasTexcoords ? glm::vec2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y) : glm::vec2(0.0f);
        }

        for(unsigned int j = 0; j < mesh->mNumFaces; j++)
        {
            aiFace face = mesh->mFaces[j];
            indices[j * 3 + 0] = face.mIndices[0];
            indices[j * 3 + 1] = face.mIndices[1];
            indices[j * 3 + 2] = face.mIndices[2];
        }

        SubMesh submesh = meshRegistry.Register(vertices, indices);
        subMeshes.push_back(submesh);

        renderer.AddMesh(submesh, materials[materialOffset + i]);
    }

    DebugLayer::Log(DebugLayer::LogType::INFO, "Loaded " + std::to_string(subMeshes.size()) + " submeshes with " + std::to_string(vertexCount) + " vertices and " + std::to_string(indexCount) + " indices in " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(t3 - t2).count()) + "s");
    DebugLayer::Log(DebugLayer::LogType::INFO, "Loaded model in " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(t3 - t1).count()) + "s");

    return renderer;
}
