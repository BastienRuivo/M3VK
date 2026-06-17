#include "rendering/Shaders/Shader.h"
#include "application/ApplicationInfo.h"
#include "application/ApplicationHelper.h"
#include <cstdint>
#include <filesystem>

#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "spirv_reflect.h"

void Shader::Dispose()
{
    VkFunctions::vkDestroyShaderEXT(Handle, nullptr);
    Handle = VK_NULL_HANDLE;
    Stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    Type = ShaderType::Count;
}

void Shader::Init(const std::filesystem::path& path, VkShaderStageFlagBits stageFlags, std::span<const VkDescriptorSetLayout> descriptorLayouts, std::span<const VkPushConstantRange> pushConstantRanges, std::span<const SpecializationConstant> speConstants)
{
    Stage = stageFlags;
    switch(stageFlags)
    {
        case VK_SHADER_STAGE_VERTEX_BIT: Type = ShaderType::Vertex; break;
        case VK_SHADER_STAGE_FRAGMENT_BIT: Type = ShaderType::Fragment; break;
        case VK_SHADER_STAGE_COMPUTE_BIT: Type = ShaderType::Compute; break;
        default: throw std::runtime_error("Unsupported shader type");
    }

    std::vector<char> shaderCode = ApplicationHelper::ReadFile(path);

    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(shaderCode.size(), shaderCode.data(), &module);
    if(result != SPV_REFLECT_RESULT_SUCCESS)
    {
        throw std::runtime_error("Can't reflect shader code");
    }

    uint32_t speConstantCount = 0;
    spvReflectEnumerateSpecializationConstants(&module, &speConstantCount, nullptr);

    std::vector<SpvReflectSpecializationConstant*> constants(speConstantCount);
    spvReflectEnumerateSpecializationConstants(&module, &speConstantCount, constants.data());

    std::vector<VkSpecializationMapEntry> mapEntries(speConstantCount);
    std::vector<VkBool32> mapValues(speConstantCount);

    VkDeviceSize offset = 0;
    for(uint32_t i = 0; i < constants.size(); ++i)
    {
        auto& entry = mapEntries[i];
        const auto& constant = constants[i];

        mapValues[i] = *static_cast<VkBool32*>(constant->default_value);
        for(uint32_t j = 0; j < speConstants.size(); ++j)
        {
            if(speConstants[j].name == constant->name)
            {
                mapValues[i] = static_cast<VkBool32>(speConstants[j].enabled);
                break;
            }
        }

        entry.constantID = constant->constant_id;
        entry.offset = offset;
        entry.size = constant->default_value_size;

        offset += entry.size;
    }

    VkSpecializationInfo speInfos
    {
        .mapEntryCount = static_cast<uint32_t>(mapEntries.size()),
        .pMapEntries = mapEntries.data(),
        .dataSize = mapValues.size() * sizeof(VkBool32),
        .pData = mapValues.data(),
    };

    if(Type == ShaderType::Compute)
    {
        auto entryPoint = spvReflectGetEntryPoint(&module, "main");

        Info.Compute.X = entryPoint->local_size.x;
        Info.Compute.Y = entryPoint->local_size.y;
        Info.Compute.Z = entryPoint->local_size.z;

        if(Info.Compute.X == 0 || Info.Compute.Y == 0 || Info.Compute.Z == 0) throw std::runtime_error("Try to load a kernel with size 0 !");
    }

    spvReflectDestroyShaderModule(&module);

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
        .pSpecializationInfo = speConstantCount > 0 ? &speInfos : nullptr
    };

    Handle = VK_NULL_HANDLE;
    if(VkFunctions::vkCreateShadersEXT(1, &createInfo, nullptr, &Handle) != VK_SUCCESS)
    {
        throw std::runtime_error("Can't compile shader code");
    }
}
