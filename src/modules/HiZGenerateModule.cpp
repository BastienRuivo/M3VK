#include "modules/HiZGenerateModule.h"
#include "ShaderBindings.h"
#include "allocation/BindingManager.h"
#include "allocation/RessourceUsage.h"
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
    swapChain.GetExtent().width >> 1, swapChain.GetExtent().height >> 1,
    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    VK_FORMAT_R32_SFLOAT)),
    _hizImageViews(_hizTexture, bindingManager),
    _hizImageSet(bindingManager, _hizTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, samplerNearest)
{
    Shader::SpecializationConstant constants[] = {Shader::SpecializationConstant{"USE_ORIGINAL_DEPTH", false}};
    uint32_t hizGenerateShader = shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "hizgenerate.comp.spv", ShaderLibrary::Compute, bindingManager, constants);
    auto& hizGenerateShaderInfo = shaderLibrary.Get(hizGenerateShader);
    _hizGenerateKernel =
    {
        .LibraryIndex = hizGenerateShader,
        .Handle = hizGenerateShaderInfo.Handle,
        .GX = hizGenerateShaderInfo.Info.Compute.X,
        .GY = hizGenerateShaderInfo.Info.Compute.Y,
        .GZ = hizGenerateShaderInfo.Info.Compute.Z
    };

    constants[0].enabled = true;
    uint32_t hizGenerateMip0Shader = shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "hizgenerate.comp.spv", ShaderLibrary::Compute, bindingManager, constants);
    auto& hizGenerateMip0ShaderInfo = shaderLibrary.Get(hizGenerateMip0Shader);
    _hizGenerateMip0Kernel =
    {
        .LibraryIndex = hizGenerateMip0Shader,
        .Handle = hizGenerateMip0ShaderInfo.Handle,
        .GX = hizGenerateMip0ShaderInfo.Info.Compute.X,
        .GY = hizGenerateMip0ShaderInfo.Info.Compute.Y,
        .GZ = hizGenerateMip0ShaderInfo.Info.Compute.Z
    };

    _hizImageViews.Bind(bindingManager, BINDING_HIZ_TEXTURE);
}

void HiZGenerateModule::Execute(const CommandBuffer& cmdBuffer, const BindingManager& manager, const BindlessTexture& depthTarget,VkPipelineLayout layout) const
{
    const auto & hiz = _hizTexture.Texture(manager).Internal();
    const auto & depth = depthTarget.Texture(manager).Internal();

    ComputeConstants constants =
    {
        .srcPixelSize = glm::vec2(1.0f / hiz.Width, 1.0f / hiz.Height),
        .srcSize = glm::uvec2(hiz.Width, hiz.Height),

        .dstPixelSize = glm::vec2(1.0f / hiz.Width, 1.0f / hiz.Height),
        .dstSize = glm::uvec2(hiz.Width, hiz.Height),

        .srcIndex = _hizTexture.CurrentIndex(),
        .dstIndex = _hizTexture.CurrentIndex()
    };

    uint32_t previousWidth = depth.Width;
    uint32_t previousHeight = depth.Height;
    uint32_t currentWidth = hiz.Width;
    uint32_t currentHeight = hiz.Height;

    cmdBuffer.BeginMarker("HiZGenerateModule");
    {
        ImageHelper::TransitionLayoutCommand(cmdBuffer, depth, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);

        for(int i = 0; i < hiz.MipCount; i++)
        {
            ImageHelper::StorageImageReadWriteCommand(cmdBuffer, hiz, true, i, 1, 0, 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            bool useOriginalDepth = i == 0;

            if(useOriginalDepth) _hizGenerateMip0Kernel.Bind(cmdBuffer);
            else _hizGenerateKernel.Bind(cmdBuffer);

            constants.srcIndex = useOriginalDepth ? depthTarget.CurrentBindlessIndex() : _hizImageViews.CurrentIndex(i - 1);
            constants.dstIndex = _hizImageViews.CurrentIndex(i);

            constants.srcSize = glm::uvec2(currentWidth, currentHeight);
            constants.srcPixelSize = glm::vec2(1.0f / currentWidth, 1.0f / currentHeight);
            constants.dstSize = glm::uvec2(currentWidth, currentHeight);
            constants.dstPixelSize = glm::vec2(1.0f / currentWidth, 1.0f / currentHeight);

            cmdBuffer.PushConstants(layout, sizeof(CommonIndexes), sizeof(ComputeConstants), &constants);

            if(useOriginalDepth) _hizGenerateMip0Kernel.CeilDispatch(cmdBuffer, currentWidth, currentHeight);
            else _hizGenerateKernel.CeilDispatch(cmdBuffer, currentWidth, currentHeight);

            ImageHelper::StorageImageReadWriteCommand(cmdBuffer, hiz, false, i, 1, 0, 1);
            previousWidth = currentWidth;
            previousHeight = currentHeight;

            currentWidth >>= 1;
            currentHeight >>= 1;
        }

        ImageHelper::StorageImageGeneralToLayoutCommand(cmdBuffer, hiz, false, 0, hiz.MipCount, 0, 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        ImageHelper::TransitionLayoutCommand(cmdBuffer, depth, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    }
    cmdBuffer.EndMarker();
}

void HiZGenerateModule::DoUI(const UserInterface& ui) const
{
    ImGui::Begin("HiZ Generate");
    {
        ImGui::Image((ImTextureID)_hizImageSet.Current(), ImVec2(512, 512));
    }
    ImGui::End();
}

void HiZGenerateModule::Resize(const CommandBuffer& cmdBuffer, uint32_t width, uint32_t height)
{

}

HiZGenerateModule::~HiZGenerateModule()
{
}
