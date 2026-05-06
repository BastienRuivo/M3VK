#pragma once

#include <filesystem>
#include <vector>
#include <vulkan/vulkan_core.h>
class Shader
{
    public:
    Shader(const std::filesystem::path& path, VkShaderStageFlags stageFlags);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    VkShaderModule CreateShaderModule();
    inline VkShaderModule Internal() const { return _internal; }

    private:
    VkShaderModule _internal;
};
