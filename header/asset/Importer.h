#pragma once

#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

enum TextureType
{
    BaseColor = 0,
    NormalMap = 1,
    MRAO = 2
};

struct TextureImport
{
    uint32_t Type;
    uint32_t Offset;
    uint32_t Size;
    uint32_t Width;
    uint32_t Height;
    VkFormat Format;
    uint32_t MipCount;
};

struct SubMeshImport
{
    uint32_t MaterialIndex;
    uint32_t VertexOffset;
    uint32_t VertexCount;
    uint32_t IndexOffset;
    uint32_t IndexCount;
    glm::vec3 AABBMin;
    glm::vec3 AABBMax;
};

struct MaterialImport
{
    uint32_t BaseColorTexId;
    uint32_t NormalMapTexId;
    uint32_t MRAOTexId;
    uint32_t MaterialType;

    glm::vec4 BaseColor;
    float Metallic;
    float Roughness;
};

struct Importer
{
    uint32_t Version;
    virtual bool Load(const std::filesystem::path& path) = 0;
    virtual void Clear() = 0;
};
