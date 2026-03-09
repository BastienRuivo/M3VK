#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>
class Shader
{
    public:
    Shader(const VkDevice& device);
    ~Shader();

    VkShaderModule CreateShaderModule(const std::vector<char>& shaderCode);
    VkShaderModule VertexShader;
    VkShaderModule FragmentShader;

    private:
    VkDevice _device;
};
