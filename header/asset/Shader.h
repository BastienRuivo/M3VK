#pragma once

#include <filesystem>
#include <span>
#include <vulkan/vulkan_core.h>
#include "rendering/ShaderState.h"
class Shader
{
    public:
    Shader(const std::filesystem::path& path, ShaderState state, VkShaderStageFlagBits stageFlags, std::span<const VkDescriptorSetLayout> descriptorLayouts, std::span<const VkPushConstantRange> pushConstantRanges);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    VkShaderEXT CreateShader();
    inline VkShaderEXT Internal() const { return _internal; }
    inline void Bind(const CommandBuffer& cmdBuffer) const
    {
        cmdBuffer.BindShaders(1, &_stage, &_internal);
        _state.Bind(cmdBuffer);
    }

    private:
    VkShaderEXT _internal;
    VkShaderStageFlagBits _stage;
    ShaderState _state;
};
