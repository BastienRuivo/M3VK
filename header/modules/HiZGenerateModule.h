#pragma once

#include "rendering/CommandBuffer.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

class HiZGenerateModule
{
public:

    struct CullingConstants
    {
        uint32_t InstanceCount;
        uint32_t DrawCount;
    };

    HiZGenerateModule(ShaderLibrary& shaderLibrary, BindingManager& descriptorAllocator);
    ~HiZGenerateModule();

    void Execute(const CommandBuffer& cmdBuffer, const GPUImage& depthTarget, VkPipelineLayout layout) const;
    inline const BindlessTexture& HiZTexture() const { return _hizTexture; }

private:
    BindlessTexture _hizTexture;
    ShaderLibrary::ComputeKernel _hizGenerateKernel;
};
