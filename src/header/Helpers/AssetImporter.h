#pragma once


#include "assimp/Importer.hpp"
#include "assimp/material.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"


#include "header/CPUImage.h"
#include "header/DescriptorPool.h"
#include "header/GPUImage.h"
#include "header/Registries/MaterialRegistry.h"
#include "header/Registries/MeshRegistry.h"
#include "header/Renderer.h"
#include "header/Vertex.h"
#include <filesystem>

namespace AssetImporter {
    static aiTextureType SelectTextureType(std::span<const aiTextureType> types, const aiMaterial* material, uint32_t& textureCount)
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

    static Renderer Load3DModel(const std::string & modelPath, MeshRegistry & meshRegistry, MaterialRegistry & materialRegistry, std::vector<GPUAllocatedImage> & textures, std::vector<Material> & materials, int defaultMaterial, DescriptorPool& descriptorPool, VkCommandPool cmdPool, VkQueue queue, VkSampler sampler)
    {
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(modelPath,
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_GenUVCoords |
            aiProcess_FlipUVs);

        if(scene == nullptr)
        {
            DebugLayer::Log(DebugLayer::LogType::ERROR, importer.GetErrorString());
            DebugLayer::Log(DebugLayer::LogType::ERROR, "Can't open " + std::string(std::filesystem::current_path()) + "/" + modelPath);
            throw std::runtime_error(importer.GetErrorString());
        }

        int materialOffset = materials.size();

        for(unsigned int i = 0; i < scene->mNumMaterials; i++)
        {
            aiMaterial* material = scene->mMaterials[i];

            ImageHelper::ImageBinding albedoMap = materials[defaultMaterial].AlbedoMap;

            // get base color value
            aiColor4D color;
            material->Get(AI_MATKEY_COLOR_DIFFUSE, color);

            GPUMaterial gpuMaterial
            {
                .Albedo = glm::vec4(color.r, color.g, color.b, color.a)
            };

            bool hasProperties = gpuMaterial != GPUMaterial::Default();

            bool hasTexture = false;

            uint32_t textureCount = 0;
            aiTextureType textureType = SelectTextureType({{ aiTextureType::aiTextureType_DIFFUSE, aiTextureType::aiTextureType_BASE_COLOR, aiTextureType::aiTextureType_UNKNOWN }}, material, textureCount);

            if(textureCount > 0)
            {
                aiString path;
                material->GetTexture(textureType, 0, &path);

                if(path.length > 0)
                {
                    std::filesystem::path texturePath(modelPath);
                    texturePath = texturePath.remove_filename();

                    std::string rawPath = std::string(path.C_Str());
                    std::replace(rawPath.begin(), rawPath.end(), '\\', '/');

                    texturePath = texturePath.append(rawPath);
                    texturePath = texturePath.lexically_normal();

                    if(!std::filesystem::exists(texturePath))
                    {
                        DebugLayer::Log(DebugLayer::LogType::WARNING, "Path does not exist " + texturePath.string());

                    }
                    else
                    {
                        auto& texture = textures.emplace_back(CPUImage(texturePath, STBI_rgb_alpha), cmdPool, queue);
                        albedoMap = ImageHelper::ImageBinding(texture.Internal(), sampler);
                        hasTexture = true;
                    }
                }
                else
                {
                    DebugLayer::Log(DebugLayer::LogType::WARNING, "No path found for texture" + std::to_string(i) + " of type " + std::to_string(textureType) + " for material " + std::to_string(i) + " of object " + modelPath);
                }
            }

            if(!hasTexture && !hasProperties)
            {
                DebugLayer::Log(DebugLayer::LogType::WARNING, "Fallback to default material for material at index " + std::to_string(i) + " of object " + modelPath);
                materials.emplace_back(materials[defaultMaterial]);
            }
            else
            {
                BufferHelper::BufferBinding binding = materialRegistry.Register(gpuMaterial);
                materials.emplace_back(albedoMap, binding, descriptorPool);
            }
        }

        std::vector<SubMesh> subMeshes;
        Renderer renderer(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));

        for(unsigned int i = 0; i < scene->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[i];
            std::vector<Vertex> vertices(mesh->mNumVertices);
            std::vector<uint32_t> indices(mesh->mNumFaces * 3);

            for(unsigned int j = 0; j < mesh->mNumVertices; j++)
            {
                vertices[j].pos = glm::vec3(mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z);
                vertices[j].texCoord = glm::vec2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y);
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

        return renderer;
    }
}
