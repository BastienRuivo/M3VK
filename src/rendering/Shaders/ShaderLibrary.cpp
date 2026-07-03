#include "rendering/Shaders/ShaderLibrary.h"
#include "allocation/BindingManager.h"

ShaderLibrary::ShaderLibrary()
{

}

ShaderLibrary::~ShaderLibrary()
{
    for(auto& shader : _shaders)
    {
        shader.Dispose();
    }
}

ShaderLibrary::ShaderLibrary(ShaderLibrary&& other) noexcept
{
    _shaders = std::move(other._shaders);
}

ShaderLibrary& ShaderLibrary::operator=(ShaderLibrary&& other) noexcept
{
    if(this != &other)
    {
        _shaders = std::move(other._shaders);
    }

    return *this;
}

uint32_t ShaderLibrary::RegisterShader(std::filesystem::path path, ShaderType type, const BindingManager& descriptorAllocator, std::span<const Shader::SpecializationConstant> speConstants)
{
    return RegisterShader(path, type, descriptorAllocator.GlobalSetLayouts(), descriptorAllocator.GlobalPushConstantRanges(), speConstants);
}

uint32_t ShaderLibrary::RegisterShader(std::filesystem::path path, ShaderType type, std::span<const VkDescriptorSetLayout> descriptorLayouts, std::span<const VkPushConstantRange> pushConstantRanges, std::span<const Shader::SpecializationConstant> speConstants)
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

   Shader shader{};
   shader.Init(path, stage, descriptorLayouts, pushConstantRanges, speConstants);

   for(uint32_t i = _lastFreeIndex; i < _shaders.size(); ++i)
   {
        if(_shaders[i].Handle != VK_NULL_HANDLE) continue;
        _shaders[i] = shader;
        return i;
   }

   _shaders.push_back(shader);
   return _shaders.size() - 1;
}

void ShaderLibrary::DisposeShader(uint32_t index)
{
    _shaders[index].Dispose();
    _lastFreeIndex = _lastFreeIndex > index ? index : _lastFreeIndex;
}
