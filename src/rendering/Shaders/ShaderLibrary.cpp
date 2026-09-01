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

uint32_t ShaderLibrary::RegisterShader(std::filesystem::path path, ShaderType type, const BindingManager& descriptorAllocator, std::span<const Shader::SpecializationConstant> speConstants)
{
    return RegisterShader(path, type, descriptorAllocator.GlobalSetLayouts(), descriptorAllocator.GlobalPushConstantRanges(), speConstants);
}

uint32_t ShaderLibrary::RegisterShader(std::filesystem::path path, ShaderType type, std::span<const vk::DescriptorSetLayout> descriptorLayouts, std::span<const vk::PushConstantRange> pushConstantRanges, std::span<const Shader::SpecializationConstant> speConstants)
{
    vk::ShaderStageFlagBits stage;

    switch (type)
    {
        case ShaderLibrary::ShaderType::Compute: stage = vk::ShaderStageFlagBits::eCompute; break;
        case ShaderLibrary::ShaderType::Vertex: stage = vk::ShaderStageFlagBits::eVertex; break;
        case ShaderLibrary::ShaderType::Fragment: stage = vk::ShaderStageFlagBits::eFragment; break;

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

ShaderLibrary::VertexBinding ShaderLibrary::RegisterVertexBinding(std::filesystem::path path, const BindingManager& descriptorAllocator, VertexState state, std::span<const Shader::SpecializationConstant> speConstants)
{
    return MakeVertexBinding(RegisterShader(path, Vertex, descriptorAllocator, speConstants), std::move(state));
}

ShaderLibrary::FragmentBinding ShaderLibrary::RegisterFragmentBinding(std::filesystem::path path, const BindingManager& descriptorAllocator, FragmentState state, std::span<const Shader::SpecializationConstant> speConstants)
{
    return MakeFragmentBinding(RegisterShader(path, Fragment, descriptorAllocator, speConstants), std::move(state));
}

ShaderLibrary::ComputeKernel ShaderLibrary::RegisterComputeKernel(std::filesystem::path path, const BindingManager& descriptorAllocator, std::span<const Shader::SpecializationConstant> speConstants)
{
    return MakeComputeKernel(RegisterShader(path, Compute, descriptorAllocator, speConstants));
}
