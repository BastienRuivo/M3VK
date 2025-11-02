#ifndef SHADER_CLASS
#define SHADER_CLASS

#include <vector>
#include <vulkan/vulkan_core.h>
class Shader
{
    public:
    void Create(const VkDevice& device);
    void Dispose(const VkDevice& device);
    VkShaderModule CreateShaderModule(const VkDevice& device, const std::vector<char>& shaderCode);
    VkShaderModule VertexShader;
    VkShaderModule FragmentShader;
};

#endif
