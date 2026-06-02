#pragma once

#include "rendering/DescriptorAllocator.h"
#include "rendering/Shaders/ShaderHandler.h"
#include "rendering/Shaders/ShaderState.h"
#include <cstdint>
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

    struct ComputeKernel
    {
        VkShaderEXT Shader;
        uint32_t GX, GY, GZ;

        void Bind(const CommandBuffer& cmdBuffer) const
        {
            const VkShaderStageFlagBits compute = VK_SHADER_STAGE_COMPUTE_BIT;
            cmdBuffer.BindShaders(1, &compute, &Shader);
        }

        void CeilDispatch(const CommandBuffer& cmdBuffer, uint32_t x = 1, uint32_t y = 1, uint32_t z = 1) const
        {
            if(x == 0 || y == 0 || z == 0)
            {
                DebugLayer::Log(DebugLayer::WARNING, "Try to dispatch with size 0 !");
                return;
            }
            uint32_t gx, gy, gz;

            gx = (x + GX - 1) / GX;
            gy = (y + GY - 1) / GY;
            gz = (z + GZ - 1) / GZ;

            cmdBuffer.Dispatch(gx, gy, gz);
        }
    };

    ShaderLibrary();
    ~ShaderLibrary();

    ShaderLibrary(const ShaderLibrary&) = delete;
    ShaderLibrary& operator=(const ShaderLibrary&) = delete;

    ShaderLibrary(ShaderLibrary&& other) noexcept;
    ShaderLibrary& operator=(ShaderLibrary&& other) noexcept;

    uint32_t RegisterShader(std::filesystem::path, ShaderType type, std::span<const VkDescriptorSetLayout> descriptorLayouts, std::span<const VkPushConstantRange> pushConstantRanges);
    uint32_t RegisterShader(std::filesystem::path, ShaderType type, const DescriptorAllocator& descriptorAllocator);
    inline const ShaderHandler& Get(uint32_t id) const { return _handlers[id]; }
    private:
    std::vector<ShaderHandler> _handlers;
};
