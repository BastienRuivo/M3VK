#include "asset/Shader.h"
#include "application/ApplicationInfo.h"
#include "application/ApplicationHelper.h"
#include <cstdint>
#include <filesystem>

#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

Shader::~Shader()
{
    VkFunctions::vkDestroyShaderEXT(_internal, nullptr);
}

Shader::Shader(const std::filesystem::path& path, ShaderState state, VkShaderStageFlagBits stageFlags, std::span<const VkDescriptorSetLayout> descriptorLayouts, std::span<const VkPushConstantRange> pushConstantRanges)
: _state(state), _stage(stageFlags)
{
    std::vector<char> shaderCode = ApplicationHelper::ReadFile(path);

    VkShaderStageFlags nextStage = 0;

    if(stageFlags == VK_SHADER_STAGE_VERTEX_BIT) nextStage = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkShaderCreateInfoEXT createInfo
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
        .flags = 0,
        .stage = stageFlags,
        .nextStage =  nextStage,// TODO
        .codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT,
        .codeSize = shaderCode.size(),
        .pCode = reinterpret_cast<const uint32_t*>(shaderCode.data()),
        .pName = "main",
        .setLayoutCount = static_cast<uint32_t>(descriptorLayouts.size()),
        .pSetLayouts = descriptorLayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
        .pPushConstantRanges = pushConstantRanges.data(),
        .pSpecializationInfo = nullptr // ???
    };

    _internal = VK_NULL_HANDLE;
    if(VkFunctions::vkCreateShadersEXT(1, &createInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Can't compile shader code");
    }
}

Shader::Shader(Shader&& other) noexcept
{
    _internal = std::exchange(other._internal, VK_NULL_HANDLE);
    _state = std::exchange(other._state, {});
    _stage = std::exchange(other._stage, VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM);
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if(this != &other)
    {
        _internal = std::exchange(other._internal, VK_NULL_HANDLE);
        _state = std::exchange(other._state, {});
        _stage = std::exchange(other._stage, VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM);
    }
    return *this;
}
