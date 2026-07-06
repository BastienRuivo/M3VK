#include "modules/HiZGenerateModule.h"
#include "ShaderBindings.h"
#include "allocation/BindingManager.h"
#include "glm/fwd.hpp"
#include "imgui.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GPUImage.h"
#include "rendering/ImageHelper.h"
#include "rendering/SwapChain.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

HiZGenerateModule::HiZGenerateModule(const SwapChain& swapChain, ShaderLibrary& shaderLibrary, BindingManager& bindingManager, VkCommandPool graphicsCommandPool, VkQueue graphicsComputeQueue, VkSampler samplerNearest)
: _hizTexture(BindlessTexture::Register(bindingManager,RessourceUsage::PerFrame, samplerNearest, graphicsCommandPool, graphicsComputeQueue,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    swapChain.GetExtent().width, swapChain.GetExtent().height,
    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    VK_FORMAT_R32_SFLOAT)),
    _hizImageSet(bindingManager, _hizTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, samplerNearest)
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

    _hizTexture.Bind(bindingManager, true, BINDING_HIZ_TEXTURE, samplerNearest);
}

void HiZGenerateModule::Execute(const CommandBuffer& cmdBuffer, const BindingManager& manager, const ImageReference& depthTarget,VkPipelineLayout layout) const
{
    const auto & hiz = _hizTexture.Texture(manager).Internal();

    ComputeConstants constants =
    {
        .srcPixelSize = glm::vec2(1.0f / hiz.Width, 1.0f / hiz.Height),
        .srcSize = glm::uvec2(hiz.Width, hiz.Height),

        .dstPixelSize = glm::vec2(1.0f / hiz.Width, 1.0f / hiz.Height),
        .dstSize = glm::uvec2(hiz.Width, hiz.Height),

        .srcIndex = _hizTexture.CurrentIndex(),
        .dstIndex = _hizTexture.CurrentIndex()
    };

    cmdBuffer.BeginMarker("HiZGenerateModule");
    {
        _hizGenerateKernel.Bind(cmdBuffer);
        ImageHelper::TransitionLayoutCommand(cmdBuffer, depthTarget, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);
        ImageHelper::StorageImageReadWriteCommand(cmdBuffer, hiz, true, 0, 1, 0, 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        cmdBuffer.PushConstants(layout, sizeof(CommonIndexes), sizeof(ComputeConstants), &constants);
        _hizGenerateKernel.CeilDispatch(cmdBuffer, hiz.Width, hiz.Height);

        ImageHelper::StorageImageGeneralToLayoutCommand(cmdBuffer, hiz, true, 0, 1, 0, 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        ImageHelper::TransitionLayoutCommand(cmdBuffer, depthTarget, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    }
    cmdBuffer.EndMarker();
}

void HiZGenerateModule::DoUI(const UserInterface& ui) const
{
    ImGui::Begin("HiZ Generate");
    {
        ImGui::Image(_hizImageSet.Current(), ImVec2(512, 512));
    }
    ImGui::End();
}

void HiZGenerateModule::Resize(const CommandBuffer& cmdBuffer, uint32_t width, uint32_t height)
{

}
