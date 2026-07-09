#pragma once

#include "allocation/BindingManager.h"
#include "application/UserInterface.h"
#include "glm/fwd.hpp"
#include "modules/Module.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/PerMipImageView.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include "rendering/SwapChain.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

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
    };

    HiZGenerateModule(const SwapChain& swapChain, ShaderLibrary& shaderLibrary, BindingManager& bindingManager, VkCommandPool graphicsCommandPool, VkQueue graphicsComputeQueue, VkSampler samplerNearest);
    ~HiZGenerateModule() override;

    void Execute(const CommandBuffer& cmdBuffer, const BindingManager& bindingManager, const BindlessTexture& depthTarget, VkPipelineLayout layout) const;
    void Resize(const CommandBuffer& cmdBuffer, uint32_t width, uint32_t height) override;
    void DoUI(const UserInterface& ui) const override;
    inline const BindlessTexture& HiZTexture() const { return _hizTexture; }


private:
    BindlessTexture _hizTexture;
    MultiFramePerMipImageView _hizImageViews;
    UserInterfaceImageSet _hizImageSet;
    ShaderLibrary::ComputeKernel _hizGenerateMip0Kernel;
    ShaderLibrary::ComputeKernel _hizGenerateKernel;

};
