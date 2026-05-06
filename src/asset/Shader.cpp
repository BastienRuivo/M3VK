#include "asset/Shader.h"
#include "application/ApplicationInfo.h"
#include "application/ApplicationHelper.h"
#include <filesystem>

#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

Shader::~Shader()
{
    vkDestroyShaderModule(ApplicationInfo::Device(), _internal, nullptr);
}

Shader::Shader(const std::filesystem::path& path, VkShaderStageFlags stageFlags)
{
    std::vector<char> shaderCode = ApplicationHelper::ReadFile(path);

    VkShaderModuleCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .flags = stageFlags,
        .codeSize = shaderCode.size(),
        .pCode = reinterpret_cast<const uint32_t*>(shaderCode.data())
    };

    _internal = VK_NULL_HANDLE;
    if(vkCreateShaderModule(ApplicationInfo::Device(), &createInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Can't compile shader code");
    }
}

Shader::Shader(Shader&& other) noexcept
{
    _internal = std::exchange(other._internal, VK_NULL_HANDLE);
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if(this != &other)
    {
        _internal = std::exchange(other._internal, VK_NULL_HANDLE);
    }
    return *this;
}
