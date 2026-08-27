#include "rendering/Shaders/Shader.h"
#include "application/ApplicationInfo.h"
#include "application/ApplicationHelper.h"
#include <cstdint>
#include <filesystem>

#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "spirv_reflect.h"

void Shader::Dispose()
{
    vk::Device(ApplicationInfo::Device()).destroyShaderEXT(Handle);
    Handle = VK_NULL_HANDLE;
    Stage = vk::ShaderStageFlagBits::eAll;
    Type = ShaderType::Count;
}

void Shader::Init(const std::filesystem::path& path, vk::ShaderStageFlagBits stageFlags, std::span<const vk::DescriptorSetLayout> descriptorLayouts, std::span<const vk::PushConstantRange> pushConstantRanges, std::span<const SpecializationConstant> speConstants)
{
    Stage = stageFlags;
    switch(stageFlags)
    {
        case vk::ShaderStageFlagBits::eVertex: Type = ShaderType::Vertex; break;
        case vk::ShaderStageFlagBits::eFragment: Type = ShaderType::Fragment; break;
        case vk::ShaderStageFlagBits::eCompute: Type = ShaderType::Compute; break;
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

    std::vector<vk::SpecializationMapEntry> mapEntries(speConstantCount);
    std::vector<vk::Bool32> mapValues(speConstantCount);

    vk::DeviceSize offset = 0;
    for(uint32_t i = 0; i < constants.size(); ++i)
    {
        auto& entry = mapEntries[i];
        const auto& constant = constants[i];

        mapValues[i] = *static_cast<vk::Bool32*>(constant->default_value);
        for(uint32_t j = 0; j < speConstants.size(); ++j)
        {
            if(speConstants[j].name == constant->name)
            {
                mapValues[i] = static_cast<vk::Bool32>(speConstants[j].enabled);
                break;
            }
        }

        entry.setConstantID(constant->constant_id)
            .setOffset(offset)
            .setSize(constant->default_value_size);

        offset += entry.size;
    }

    vk::SpecializationInfo speInfos = vk::SpecializationInfo{}
        .setMapEntries(mapEntries)
        .setDataSize(mapValues.size() * sizeof(vk::Bool32))
        .setPData(mapValues.data());

    if(Type == ShaderType::Compute)
    {
        auto entryPoint = spvReflectGetEntryPoint(&module, "main");

        Info.Compute.X = entryPoint->local_size.x;
        Info.Compute.Y = entryPoint->local_size.y;
        Info.Compute.Z = entryPoint->local_size.z;

        if(Info.Compute.X == 0 || Info.Compute.Y == 0 || Info.Compute.Z == 0) throw std::runtime_error("Try to load a kernel with size 0 !");
    }

    spvReflectDestroyShaderModule(&module);

    vk::ShaderStageFlags nextStage = {};

    if(stageFlags == vk::ShaderStageFlagBits::eVertex) nextStage = vk::ShaderStageFlagBits::eFragment;

    std::span<const uint32_t> spirvCode(reinterpret_cast<const uint32_t*>(shaderCode.data()), shaderCode.size() / sizeof(uint32_t));

    vk::ShaderCreateInfoEXT createInfo = vk::ShaderCreateInfoEXT{}
        .setStage(stageFlags)
        .setNextStage(nextStage) // TODO
        .setCodeType(vk::ShaderCodeTypeEXT::eSpirv)
        .setCode<uint32_t>(spirvCode)
        .setPName("main")
        .setSetLayouts(descriptorLayouts)
        .setPushConstantRanges(pushConstantRanges)
        .setPSpecializationInfo(speConstantCount > 0 ? &speInfos : nullptr);

    Handle = VK_NULL_HANDLE;
    if(vk::Device(ApplicationInfo::Device()).createShadersEXT(1, &createInfo, nullptr, &Handle) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Can't compile shader code");
    }
}
