#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vulkan/vulkan.hpp>

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

    void Init(const std::filesystem::path& path, vk::ShaderStageFlagBits stageFlags, std::span<const vk::DescriptorSetLayout> descriptorLayouts, std::span<const vk::PushConstantRange> pushConstantRanges, std::span<const SpecializationConstant> speConstants);
    void Dispose();

    vk::ShaderEXT Handle;
    vk::ShaderStageFlagBits Stage;
    ShaderType Type;
    ShaderInfo Info;
};
