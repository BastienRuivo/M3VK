#include "./header/Shader.h"
#include "./header/M3VKHelper.h"
#include "header/VkDebugLayer.h"
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

Shader::~Shader()
{
    VkDebugLayer::LogInfo("Shader Destroyed !");
    vkDestroyShaderModule(_device, VertexShader, nullptr);
    vkDestroyShaderModule(_device, FragmentShader, nullptr);
}

VkShaderModule Shader::CreateShaderModule(const std::vector<char>& shaderCode)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    VkShaderModule module{};
    if(vkCreateShaderModule(_device, &createInfo, nullptr, &module) != VK_SUCCESS)
    {
        throw std::runtime_error("Can't compile shader code");
    }

    return module;
}

Shader::Shader(const VkDevice& device)
{
    VkDebugLayer::LogInfo("Shader Created !");
    std::vector<char> vertex = M3VKHelper::ReadFile("shaders/helloTriangle.vert.spv");
    std::vector<char> fragment = M3VKHelper::ReadFile("shaders/helloTriangle.frag.spv");

    _device = device;

    VertexShader = CreateShaderModule(vertex);
    FragmentShader = CreateShaderModule(fragment);
}
