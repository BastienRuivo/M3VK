#include "modules/SkyboxModule.h"
#include "asset/ImporterHelper.h"
#include "allocation/BindingManager.h"
#include "allocation/RaiiHelper.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include <vulkan/vulkan.hpp>
#include "ShaderBindings.h"
#include "vulkan/vulkan.hpp"

SkyboxModule::SkyboxModule(ShaderLibrary& shaderLibrary, BindingManager& allocator, vk::CommandPool pool, vk::Queue queue)
:
    _sampler(RaiiHelper::MakeSampler()),
    _skyboxTexture(ImporterHelper::CubemapFromCPU(allocator, BINDING_SKYBOX_TEXTURE, pool, queue, _sampler,
        CPUImage("data/skybox/front.jpg", STBI_rgb_alpha),
        CPUImage("data/skybox/back.jpg", STBI_rgb_alpha),
        CPUImage("data/skybox/left.jpg", STBI_rgb_alpha),
        CPUImage("data/skybox/right.jpg", STBI_rgb_alpha),
        CPUImage("data/skybox/top.jpg", STBI_rgb_alpha),
        CPUImage("data/skybox/bottom.jpg", STBI_rgb_alpha)))
{
    _vertexShader = shaderLibrary.RegisterVertexBinding(std::filesystem::path(SHADER_DIRECTORY) / "Skybox.vert.spv", allocator,
        VertexState{.CullMode = vk::CullModeFlagBits::eNone});

    _fragmentShader = shaderLibrary.RegisterFragmentBinding(std::filesystem::path(SHADER_DIRECTORY) / "Skybox.frag.spv", allocator,
        FragmentState{.depthTest = true, .depthCompareOp = vk::CompareOp::eLessOrEqual, .depthWrite = false});
}

SkyboxModule::~SkyboxModule()
{

}

void SkyboxModule::Execute(const CommandBuffer& cmdBuffer, vk::PipelineLayout layout) const
{
    cmdBuffer.BeginMarker("Skybox");
    {
        _vertexShader.Bind(cmdBuffer);
        _fragmentShader.Bind(cmdBuffer);

        cmdBuffer.Draw(3, 1, 0, 0);
    }
    cmdBuffer.EndMarker();
}
