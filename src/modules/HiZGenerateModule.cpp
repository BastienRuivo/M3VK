#include "modules/HiZGenerateModule.h"
#include "ShaderBindings.h"
#include "allocation/BindingManager.h"
#include "allocation/RessourceUsage.h"
#include "imgui.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GPUImage.h"
#include "rendering/ImageHelper.h"
#include "rendering/SwapChain.h"
#include <cstdint>
#include <vulkan/vulkan.hpp>

HiZGenerateModule::HiZGenerateModule(const SwapChain& swapChain, ShaderLibrary& shaderLibrary, BindingManager& bindingManager, vk::CommandPool graphicsCommandPool, vk::Queue graphicsComputeQueue, vk::Sampler samplerNearest)
: _hizTexture(BindlessTexture::Register(bindingManager,RessourceUsage::PerFrame, samplerNearest, graphicsCommandPool, graphicsComputeQueue,
    vk::ImageLayout::eShaderReadOnlyOptimal,
    swapChain.GetExtent().width >> 1, swapChain.GetExtent().height >> 1,
    vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc,
    vk::Format::eR32Sfloat)),
    _hizImageViews(_hizTexture, bindingManager)
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

void HiZGenerateModule::Execute(const CommandBuffer& cmdBuffer, const BindingManager& manager, const BindlessTexture& depthTarget,vk::PipelineLayout layout) const
{
    const auto & hiz = _hizTexture.Texture(manager).Internal();
    const auto & depth = depthTarget.Texture(manager).Internal();

    ComputeConstants constants;

    uint32_t previousWidth = depth.Width;
    uint32_t previousHeight = depth.Height;
    uint32_t currentWidth = hiz.Width;
    uint32_t currentHeight = hiz.Height;

    cmdBuffer.BeginMarker("HiZGenerateModule");
    {
        ImageHelper::TransitionLayoutCommand(cmdBuffer, depth, vk::ImageLayout::eDepthAttachmentOptimal, vk::ImageLayout::eDepthReadOnlyOptimal);

        for(int i = 0; i < hiz.MipCount; i++)
        {
            ImageHelper::StorageImageReadWriteCommand(cmdBuffer, hiz, true, i, 1, 0, 1, vk::ImageLayout::eShaderReadOnlyOptimal);
            bool useOriginalDepth = i == 0;

            if(useOriginalDepth) _hizGenerateMip0Kernel.Bind(cmdBuffer);
            else _hizGenerateKernel.Bind(cmdBuffer);

            constants.srcIndex = useOriginalDepth ? depthTarget.CurrentBindlessIndex() : _hizTexture.CurrentBindlessIndex();
            constants.dstIndex = _hizImageViews.CurrentIndex(i);

            constants.srcSize = glm::uvec2(previousWidth, previousHeight);
            constants.srcPixelSize = glm::vec2(1.0f / previousWidth, 1.0f / previousHeight);
            constants.dstSize = glm::uvec2(currentWidth, currentHeight);
            constants.dstPixelSize = glm::vec2(1.0f / currentWidth, 1.0f / currentHeight);
            constants.srcMipLevel = useOriginalDepth ? 0 : i - 1;

            cmdBuffer.PushConstants(layout, COMMON_INDEXES_OFFSET, sizeof(ComputeConstants), &constants);

            if(useOriginalDepth) _hizGenerateMip0Kernel.CeilDispatch(cmdBuffer, currentWidth, currentHeight);
            else _hizGenerateKernel.CeilDispatch(cmdBuffer, currentWidth, currentHeight);

            ImageHelper::StorageImageGeneralToLayoutCommand(cmdBuffer, hiz, true, i, 1, 0, 1, vk::ImageLayout::eShaderReadOnlyOptimal);
            previousWidth = currentWidth;
            previousHeight = currentHeight;

            currentWidth = std::max(currentWidth >> 1, 1u);
            currentHeight = std::max(currentHeight >> 1, 1u);
        }

        //ImageHelper::StorageImageGeneralToLayoutCommand(cmdBuffer, hiz, false, 0, hiz.MipCount, 0, 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        ImageHelper::TransitionLayoutCommand(cmdBuffer, depth, vk::ImageLayout::eDepthReadOnlyOptimal, vk::ImageLayout::eDepthAttachmentOptimal);
    }
    cmdBuffer.EndMarker();
}

void HiZGenerateModule::Resize(const CommandBuffer& cmdBuffer, BindingManager& bindingManager, uint32_t width, uint32_t height)
{
    if(_hizTexture.Resize(bindingManager, width >> 1, height >> 1))
    {
        _hizImageViews = MultiFramePerMipImageView(_hizTexture, bindingManager);
        _hizImageViews.Bind(bindingManager, BINDING_HIZ_TEXTURE);
        _hizTexture.TransistionAllLayoutCommand(bindingManager, cmdBuffer, vk::ImageLayout::eShaderReadOnlyOptimal);
    }
}

void HiZGenerateModule::DoUI(const UserInterface& ui)
{
    ImGui::Begin("HiZ Generate");
    {
        ImGui::SliderInt("Hiz Mip", &_drawHiZMip, -1, 13);
    }
    ImGui::End();
}

void HiZGenerateModule::RenderUI(const CommandBuffer& cmdBuffer, const FullscreenDrawDebug& debugDrawModule, vk::PipelineLayout layout) const
{
    if(_drawHiZMip >= 0)
    {
        debugDrawModule.Draw(cmdBuffer, FullscreenDrawDebug::DrawOption_Depth, _hizTexture.CurrentBindlessIndex(), layout, _drawHiZMip);
    }
}

HiZGenerateModule::~HiZGenerateModule()
{

}
