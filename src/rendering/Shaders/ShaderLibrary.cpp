#include "rendering/Shaders/ShaderLibrary.h"
#include "rendering/DescriptorAllocator.h"

ShaderLibrary::ShaderLibrary()
{

}

ShaderLibrary::~ShaderLibrary()
{

}

ShaderLibrary::ShaderLibrary(ShaderLibrary&& other) noexcept
{
    _handlers = std::move(other._handlers);
}

ShaderLibrary& ShaderLibrary::operator=(ShaderLibrary&& other) noexcept
{
    if(this != &other)
    {
        _handlers = std::move(other._handlers);
    }

    return *this;
}

uint32_t ShaderLibrary::RegisterShader(std::filesystem::path path, ShaderType type, const DescriptorAllocator& descriptorAllocator, std::span<const ShaderHandler::SpecializationConstant> speConstants)
{
    return RegisterShader(path, type, descriptorAllocator.GlobalSetLayouts(), descriptorAllocator.GlobalPushConstantRanges(), speConstants);
}

uint32_t ShaderLibrary::RegisterShader(std::filesystem::path path, ShaderType type, std::span<const VkDescriptorSetLayout> descriptorLayouts, std::span<const VkPushConstantRange> pushConstantRanges, std::span<const ShaderHandler::SpecializationConstant> speConstants)
{
    VkShaderStageFlagBits stage;

    switch (type)
    {
        case ShaderLibrary::ShaderType::Compute: stage = VK_SHADER_STAGE_COMPUTE_BIT; break;
        case ShaderLibrary::ShaderType::Vertex: stage = VK_SHADER_STAGE_VERTEX_BIT; break;
        case ShaderLibrary::ShaderType::Fragment: stage = VK_SHADER_STAGE_FRAGMENT_BIT; break;

        default:
        {
            throw std::runtime_error("Unsupported shader type !");
        }
    }

   _handlers.emplace_back(path, stage, descriptorLayouts, pushConstantRanges, speConstants);

   return _handlers.size() - 1;
}
