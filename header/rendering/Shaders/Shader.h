#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vulkan/vulkan_core.h>

struct Shader
{
    public:
    enum ShaderType
    {
        Vertex,
        Fragment,
        Compute,
        Count
    };

    struct SpecializationConstant
    {
        std::string name; // e.g., "ENABLE_CUTOUT"
        bool enabled;
    };

    // allocate 32 bytes of data per shader type, unused for fragment and vertex atm
    struct ComputeInfo
    {
        uint32_t X;
        uint32_t Y;
        uint32_t Z;
        uint32_t padding[5];
    };

    struct VertexInfo
    {
        uint32_t padding[8];
    };

    struct FragmentInfo
    {
        uint32_t padding[8];
    };

    union ShaderInfo
    {
        ComputeInfo Compute;
        VertexInfo Vertex;
        FragmentInfo Fragment;
    };

    void Init(const std::filesystem::path& path, VkShaderStageFlagBits stageFlags, std::span<const VkDescriptorSetLayout> descriptorLayouts, std::span<const VkPushConstantRange> pushConstantRanges, std::span<const SpecializationConstant> speConstants);
    void Dispose();

    VkShaderEXT Handle;
    VkShaderStageFlagBits Stage;
    ShaderType Type;
    ShaderInfo Info;
};
