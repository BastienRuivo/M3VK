#pragma once

#include "allocation/BindingManager.h"
#include "application/UserInterface.h"
#include "glm/fwd.hpp"
#include "modules/FullscreenDrawDebug.h"
#include "modules/Module.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/PerMipImageView.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include "rendering/SwapChain.h"
#include <cstdint>
#include <vulkan/vulkan.hpp>

class HiZGenerateModule : public Module
{
public:
    struct ComputeConstants
    {
        glm::vec2 srcPixelSize;
        glm::uvec2 srcSize;

        glm::vec2 dstPixelSize;
        glm::uvec2 dstSize;

        uint32_t srcIndex;
        uint32_t dstIndex;

        uint32_t srcMipLevel;
    };

    HiZGenerateModule(const SwapChain& swapChain, ShaderLibrary& shaderLibrary, BindingManager& bindingManager, vk::CommandPool graphicsCommandPool, vk::Queue graphicsComputeQueue, vk::Sampler samplerNearest);
    ~HiZGenerateModule() override;

    HiZGenerateModule(HiZGenerateModule&& other) noexcept;
    HiZGenerateModule& operator=(HiZGenerateModule&& other) noexcept;

    HiZGenerateModule(const HiZGenerateModule&) = delete;
    HiZGenerateModule& operator=(const HiZGenerateModule&) = delete;

    void Execute(const CommandBuffer& cmdBuffer, const BindingManager& bindingManager, const BindlessTexture& depthTarget, vk::PipelineLayout layout) const;
    void Resize(const CommandBuffer& cmdBuffer, BindingManager& bindingManager, uint32_t width, uint32_t height) override;
    void DoUI(const UserInterface& ui) override;
    void RenderUI(const CommandBuffer& cmdBuffer, const FullscreenDrawDebug& debugDrawModule, vk::PipelineLayout layout) const override;
    inline const BindlessTexture& HiZTexture() const { return _hizTexture; }

public:
    BindlessTexture _hizTexture;
    MultiFramePerMipImageView _hizImageViews;
    ShaderLibrary::ComputeKernel _hizGenerateMip0Kernel;
    ShaderLibrary::ComputeKernel _hizGenerateKernel;
    int32_t _drawHiZMip = -1;
};
