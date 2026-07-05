#include "modules/HiZGenerateModule.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GPUImage.h"
#include "rendering/SwapChain.h"

HiZGenerateModule::HiZGenerateModule(const SwapChain& swapChain, ShaderLibrary& shaderLibrary, BindingManager& bindingManager, VkCommandPool graphicsCommandPool, VkQueue graphicsComputeQueue, VkSampler samplerNearest)
: _hizTexture(BindlessTexture::Register(bindingManager,RessourceUsage::PerFrame, samplerNearest, graphicsCommandPool, graphicsComputeQueue,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        swapChain.GetExtent().width, swapChain.GetExtent().height,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        ApplicationInfo::Constant::DepthFormat))
{
        uint32_t hizGenerateShader = shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "hizgenerate.comp.spv", ShaderLibrary::Compute, bindingManager, {});

        auto& hizGenerateShaderInfo = shaderLibrary.Get(hizGenerateShader);
        _hizGenerateKernel =
        {
                .LibraryIndex = hizGenerateShader,
                .Handle = hizGenerateShaderInfo.Handle,
                .GX = hizGenerateShaderInfo.Info.Compute.X,
                .GY = hizGenerateShaderInfo.Info.Compute.Y,
                .GZ = hizGenerateShaderInfo.Info.Compute.Z
        };
}

void HiZGenerateModule::Execute(const CommandBuffer& cmdBuffer, const GPUImage& depthTarger,VkPipelineLayout layout) const
{

}

void HiZGenerateModule::DoUI(const UserInterface& ui) const
{
}

void HiZGenerateModule::Resize(const CommandBuffer& cmdBuffer, uint32_t width, uint32_t height)
{

}
