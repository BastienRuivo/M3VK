#pragma once

#include "modules/Module.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include "rendering/SwapChain.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

class HiZGenerateModule : public Module
{
public:

    struct CullingConstants
    {
        uint32_t InstanceCount;
        uint32_t DrawCount;
    };

    HiZGenerateModule(const SwapChain& swapChain, ShaderLibrary& shaderLibrary, BindingManager& bindingManager, VkCommandPool graphicsCommandPool, VkQueue graphicsComputeQueue, VkSampler samplerNearest);
    ~HiZGenerateModule() {};

    void Execute(const CommandBuffer& cmdBuffer, const GPUImage& depthTarget, VkPipelineLayout layout) const;
    void Resize(const CommandBuffer& cmdBuffer, uint32_t width, uint32_t height) override;
    void DoUI(const UserInterface& ui) const override;
    inline const BindlessTexture& HiZTexture() const { return _hizTexture; }

private:
    BindlessTexture _hizTexture;
    ShaderLibrary::ComputeKernel _hizGenerateKernel;
};
