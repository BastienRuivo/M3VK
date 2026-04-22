#include "./header/Shader.h"
#include "header/ApplicationInfo.h"
#include "header/Helpers/ProjectHelper.h"

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

Shader::~Shader()
{
    vkDestroyShaderModule(ApplicationInfo::Device(), VertexShader, nullptr);
    vkDestroyShaderModule(ApplicationInfo::Device(), FragmentShader, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "Shader Destroyed !");
#endif
}

VkShaderModule Shader::CreateShaderModule(const std::vector<char>& shaderCode)
{
    VkShaderModuleCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderCode.size(),
        .pCode = reinterpret_cast<const uint32_t*>(shaderCode.data())
    };

    VkShaderModule module{};
    if(vkCreateShaderModule(ApplicationInfo::Device(), &createInfo, nullptr, &module) != VK_SUCCESS)
    {
        throw std::runtime_error("Can't compile shader code");
    }

    return module;
}

Shader::Shader()
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "Shader Creation !");
#endif

    std::vector<char> vertex = ProjectHelper::ReadFile(std::string(SHADER_DIRECTORY) + "helloTriangle.vert.spv");
    std::vector<char> fragment = ProjectHelper::ReadFile(std::string(SHADER_DIRECTORY) + "helloTriangle.frag.spv");



    VertexShader = CreateShaderModule(vertex);
    FragmentShader = CreateShaderModule(fragment);
}
