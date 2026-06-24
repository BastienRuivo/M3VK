#include "modules/SkyboxModule.h"
#include "asset/ImporterHelper.h"
#include "rendering/DescriptorAllocator.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>
#include "ShaderBindings.h"

SkyboxModule::SkyboxModule(ShaderLibrary& shaderLibrary, DescriptorAllocator& allocator, VkCommandPool pool, VkQueue queue)
:
    _sampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR),
    _skyboxTexture(ImporterHelper::CubemapFromCPU(allocator, STATIC_BINDING_SKYBOX_TEXTURE, pool, queue, _sampler.Internal(),
        CPUImage("data/skybox/front.jpg", STBI_rgb_alpha),
        CPUImage("data/skybox/back.jpg", STBI_rgb_alpha),
        CPUImage("data/skybox/left.jpg", STBI_rgb_alpha),
        CPUImage("data/skybox/right.jpg", STBI_rgb_alpha),
        CPUImage("data/skybox/top.jpg", STBI_rgb_alpha),
        CPUImage("data/skybox/bottom.jpg", STBI_rgb_alpha)))
{
    uint32_t vertexShaderId = shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "Skybox.vert.spv", ShaderLibrary::Vertex, allocator);
    uint32_t fragmentShaderId = shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "Skybox.frag.spv", ShaderLibrary::Fragment, allocator);

    auto& vertexShaderInfo = shaderLibrary.Get(vertexShaderId);
    _vertexShader =
    {
        .LibraryIndex = vertexShaderId,
        .Handle = vertexShaderInfo.Handle,
        .State = VertexState()
    };
    _vertexShader.State.CullMode = VK_CULL_MODE_NONE;

    auto& fragmentShaderInfo = shaderLibrary.Get(fragmentShaderId);
    _fragmentShader =
    {
        .LibraryIndex = fragmentShaderId,
        .Handle = fragmentShaderInfo.Handle,
        .State = FragmentState()
    };
    _fragmentShader.State.depthTest = true;
    _fragmentShader.State.depthWrite = false;
    _fragmentShader.State.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

}

SkyboxModule::~SkyboxModule()
{

}

void SkyboxModule::Execute(const CommandBuffer& cmdBuffer, VkPipelineLayout layout) const
{
    cmdBuffer.BeginMarker("Skybox");
    {
        _vertexShader.Bind(cmdBuffer);
        _fragmentShader.Bind(cmdBuffer);

        cmdBuffer.Draw(3, 1, 0, 0);
    }
    cmdBuffer.EndMarker();
}
