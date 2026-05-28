#pragma once

#include <filesystem>
#include <span>
#include <vulkan/vulkan_core.h>
class ShaderHandler
{
    public:
    ShaderHandler(const std::filesystem::path& path, VkShaderStageFlagBits stageFlags, std::span<const VkDescriptorSetLayout> descriptorLayouts, std::span<const VkPushConstantRange> pushConstantRanges);
    ~ShaderHandler();

    ShaderHandler(const ShaderHandler&) = delete;
    ShaderHandler& operator=(const ShaderHandler&) = delete;

    ShaderHandler(ShaderHandler&& other) noexcept;
    ShaderHandler& operator=(ShaderHandler&& other) noexcept;

    VkShaderEXT CreateShader();
    inline VkShaderEXT Internal() const { return _internal; }
    inline VkShaderStageFlagBits Stage() const { return _stage; }

    private:
    VkShaderEXT _internal;
    VkShaderStageFlagBits _stage;
};
