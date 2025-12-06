#include "./header/Shader.h"
#include "./header/M3VKHelper.h"
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

void Shader::Dispose(const VkDevice& device)
{
    vkDestroyShaderModule(device, VertexShader, nullptr);
    vkDestroyShaderModule(device, FragmentShader, nullptr);
}

VkShaderModule Shader::CreateShaderModule(const VkDevice& device, const std::vector<char>& shaderCode)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    VkShaderModule module{};
    if(vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS)
    {
        throw std::runtime_error("Can't compile shader code");
    }

    return module;
}

void Shader::Create(const VkDevice& device)
{
    std::vector<char> vertex = M3VKHelper::ReadFile("shaders/helloTriangle.vert.spv");
    std::vector<char> fragment = M3VKHelper::ReadFile("shaders/helloTriangle.frag.spv");

    VertexShader = CreateShaderModule(device, vertex);
    FragmentShader = CreateShaderModule(device, fragment);


}
