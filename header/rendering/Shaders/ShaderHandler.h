#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vulkan/vulkan_core.h>

class ShaderHandler
{
    public:
    enum ShaderType
    {
        Vertex,
        Fragment,
        Compute
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

    ShaderHandler(const std::filesystem::path& path, VkShaderStageFlagBits stageFlags, std::span<const VkDescriptorSetLayout> descriptorLayouts, std::span<const VkPushConstantRange> pushConstantRanges, std::span<const SpecializationConstant> speConstants);
    ~ShaderHandler();

    ShaderHandler(const ShaderHandler&) = delete;
    ShaderHandler& operator=(const ShaderHandler&) = delete;

    ShaderHandler(ShaderHandler&& other) noexcept;
    ShaderHandler& operator=(ShaderHandler&& other) noexcept;

    inline VkShaderEXT Internal() const { return _internal; }
    inline VkShaderStageFlagBits Stage() const { return _stage; }
    inline ShaderType Type() const { return _type; }
    inline ShaderInfo Info() const { return _info; }

    private:
    VkShaderEXT _internal;
    VkShaderStageFlagBits _stage;
    ShaderType _type;
    ShaderInfo _info;
};
