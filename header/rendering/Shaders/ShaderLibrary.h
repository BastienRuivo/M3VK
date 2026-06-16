#pragma once

#include "rendering/DescriptorAllocator.h"
#include "rendering/Shaders/Shader.h"
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
        uint32_t LibraryIndex;
        VkShaderEXT Handle;
        VertexState State;
        void Bind(const CommandBuffer& cmdBuffer) const
        {
            State.Bind(cmdBuffer);
            const VkShaderStageFlagBits stage = VK_SHADER_STAGE_VERTEX_BIT;
            cmdBuffer.BindShaders(1, &stage, &Handle);
        }
    };

    struct FragmentBinding
    {
        uint32_t LibraryIndex;
        VkShaderEXT Handle;
        FragmentState State;
        void Bind(const CommandBuffer& cmdBuffer) const
        {
            State.Bind(cmdBuffer);
            const VkShaderStageFlagBits stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            cmdBuffer.BindShaders(1, &stage, &Handle);
        }
    };

    struct ComputeKernel
    {
        uint32_t LibraryIndex;
        VkShaderEXT Handle;
        uint32_t GX, GY, GZ;
        void Bind(const CommandBuffer& cmdBuffer) const
        {
            const VkShaderStageFlagBits stage = VK_SHADER_STAGE_COMPUTE_BIT;
            cmdBuffer.BindShaders(1, &stage, &Handle);
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

    uint32_t RegisterShader(std::filesystem::path, ShaderType type, std::span<const VkDescriptorSetLayout> descriptorLayouts, std::span<const VkPushConstantRange> pushConstantRanges, std::span<const Shader::SpecializationConstant> speConstants);
    uint32_t RegisterShader(std::filesystem::path, ShaderType type, const DescriptorAllocator& descriptorAllocator, std::span<const Shader::SpecializationConstant> speConstants);
    void DisposeShader(uint32_t);
    inline VkShaderEXT GetHandle(uint32_t id) const { return _shaders[id].Handle; }
    inline const Shader& Get(uint32_t id) const { return _shaders[id]; }

    private:
    std::vector<Shader> _shaders;
    uint32_t _lastFreeIndex = 0;
};
