#include "rendering/Shaders/ShaderHandler.h"
#include "application/ApplicationInfo.h"
#include "application/ApplicationHelper.h"
#include <cstdint>
#include <filesystem>

#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "spirv_reflect.h"

ShaderHandler::~ShaderHandler()
{
    VkFunctions::vkDestroyShaderEXT(_internal, nullptr);
}

ShaderHandler::ShaderHandler(const std::filesystem::path& path, VkShaderStageFlagBits stageFlags, std::span<const VkDescriptorSetLayout> descriptorLayouts, std::span<const VkPushConstantRange> pushConstantRanges)
:  _stage(stageFlags)
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

    switch(stageFlags)
    {
        case VK_SHADER_STAGE_VERTEX_BIT: _type = ShaderType::Vertex; break;
        case VK_SHADER_STAGE_FRAGMENT_BIT: _type = ShaderType::Fragment; break;
        case VK_SHADER_STAGE_COMPUTE_BIT: _type = ShaderType::Compute; break;
        default: throw std::runtime_error("Unsupported shader type");
    }

    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(shaderCode.size(), shaderCode.data(), &module);
    if(result != SPV_REFLECT_RESULT_SUCCESS)
    {
        throw std::runtime_error("Can't reflect shader code");
    }

    if(_type == ShaderType::Compute)
    {
        auto entryPoint = spvReflectGetEntryPoint(&module, "main");

        _info.Compute.X = entryPoint->local_size.x;
        _info.Compute.Y = entryPoint->local_size.y;
        _info.Compute.Z = entryPoint->local_size.z;
    }

    spvReflectDestroyShaderModule(&module);
}

ShaderHandler::ShaderHandler(ShaderHandler&& other) noexcept
{
    _internal = std::exchange(other._internal, VK_NULL_HANDLE);
    _stage = std::exchange(other._stage, VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM);
}

ShaderHandler& ShaderHandler::operator=(ShaderHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = std::exchange(other._internal, VK_NULL_HANDLE);
        _stage = std::exchange(other._stage, VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM);
    }
    return *this;
}
