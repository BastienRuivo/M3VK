#pragma once

#include "rendering/Shaders/ShaderHandler.h"
#include "rendering/Shaders/ShaderState.h"
#include <filesystem>
#include <vector>

class ShaderLibrary
{
    public:
    enum ShaderType
    {
        Vertex,
        Fragment,
        Compute
    };
    struct VertexBinding
    {
        VkShaderEXT Shader;
        VertexState State;

        void Bind(const CommandBuffer& cmdBuffer) const
        {
            State.Bind(cmdBuffer);
            const VkShaderStageFlagBits vertex = VK_SHADER_STAGE_VERTEX_BIT;
            cmdBuffer.BindShaders(1, &vertex, &Shader);
        }
    };

    struct FragmentBinding
    {
        VkShaderEXT Shader;
        FragmentState State;

        void Bind(const CommandBuffer& cmdBuffer) const
        {
            State.Bind(cmdBuffer);
            const VkShaderStageFlagBits vertex = VK_SHADER_STAGE_FRAGMENT_BIT;
            cmdBuffer.BindShaders(1, &vertex, &Shader);
        }
    };

    ShaderLibrary();
    ~ShaderLibrary();

    ShaderLibrary(const ShaderLibrary&) = delete;
    ShaderLibrary& operator=(const ShaderLibrary&) = delete;

    ShaderLibrary(ShaderLibrary&& other) noexcept;
    ShaderLibrary& operator=(ShaderLibrary&& other) noexcept;

    uint32_t RegisterShader(std::filesystem::path, ShaderType type, std::span<const VkDescriptorSetLayout> descriptorLayouts, std::span<const VkPushConstantRange> pushConstantRanges);
    inline const ShaderHandler& Get(uint32_t id) const { return _handlers[id]; }
    private:
    std::vector<ShaderHandler> _handlers;
};
