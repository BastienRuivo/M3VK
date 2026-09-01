#pragma once

#include "allocation/BindingManager.h"
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
        vk::ShaderEXT Handle;
        VertexState State;
        void Bind(const CommandBuffer& cmdBuffer) const
        {
            State.Bind(cmdBuffer);
            const vk::ShaderStageFlagBits stage = vk::ShaderStageFlagBits::eVertex;
            cmdBuffer.BindShaders({&stage, 1}, {&Handle, 1});
        }
    };

    struct FragmentBinding
    {
        uint32_t LibraryIndex;
        vk::ShaderEXT Handle;
        FragmentState State;
        void Bind(const CommandBuffer& cmdBuffer) const
        {
            State.Bind(cmdBuffer);
            const vk::ShaderStageFlagBits stage = vk::ShaderStageFlagBits::eFragment;
            cmdBuffer.BindShaders({&stage, 1}, {&Handle, 1});
        }
    };

    struct ComputeKernel
    {
        uint32_t LibraryIndex;
        vk::ShaderEXT Handle;
        uint32_t GX, GY, GZ;
        void Bind(const CommandBuffer& cmdBuffer) const
        {
            const vk::ShaderStageFlagBits stage = vk::ShaderStageFlagBits::eCompute;
            cmdBuffer.BindShaders({&stage, 1}, {&Handle, 1});
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

    ShaderLibrary(ShaderLibrary&& other) noexcept = default;
    ShaderLibrary& operator=(ShaderLibrary&& other) noexcept = default;

    uint32_t RegisterShader(std::filesystem::path, ShaderType type, std::span<const vk::DescriptorSetLayout> descriptorLayouts, std::span<const vk::PushConstantRange> pushConstantRanges, std::span<const Shader::SpecializationConstant> speConstants = {});
    uint32_t RegisterShader(std::filesystem::path, ShaderType type, const BindingManager& descriptorAllocator, std::span<const Shader::SpecializationConstant> speConstants = {});
    void DisposeShader(uint32_t);
    inline vk::ShaderEXT GetHandle(uint32_t id) const { return _shaders[id].Handle; }
    inline const Shader& Get(uint32_t id) const { return _shaders[id]; }

    private:
    std::vector<Shader> _shaders;
    uint32_t _lastFreeIndex = 0;
};
