#include "modules/SkyboxModule.h"
#include "asset/ImporterHelper.h"
#include "allocation/BindingManager.h"
#include "allocation/RaiiHelper.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include <cstdint>
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
    uint32_t vertexShaderId = shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "Skybox.vert.spv", ShaderLibrary::Vertex, allocator);
    uint32_t fragmentShaderId = shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "Skybox.frag.spv", ShaderLibrary::Fragment, allocator);

    auto& vertexShaderInfo = shaderLibrary.Get(vertexShaderId);
    _vertexShader =
    {
        .LibraryIndex = vertexShaderId,
        .Handle = vertexShaderInfo.Handle,
        .State = VertexState()
    };
    _vertexShader.State.CullMode = vk::CullModeFlagBits::eNone;

    auto& fragmentShaderInfo = shaderLibrary.Get(fragmentShaderId);
    _fragmentShader =
    {
        .LibraryIndex = fragmentShaderId,
        .Handle = fragmentShaderInfo.Handle,
        .State = FragmentState()
    };
    _fragmentShader.State.depthTest = true;
    _fragmentShader.State.depthWrite = false;
    _fragmentShader.State.depthCompareOp = vk::CompareOp::eLessOrEqual;

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

 SkyboxModule::SkyboxModule(SkyboxModule&& other) noexcept
 :  _sampler(std::move(other._sampler)),
    _skyboxTexture(std::move(other._skyboxTexture)),
    _vertexShader(std::move(other._vertexShader)),
    _fragmentShader(std::move(other._fragmentShader))
 {

 }

SkyboxModule& SkyboxModule::operator=(SkyboxModule&& other) noexcept
{
    if(this != &other)
    {
        _sampler = std::move(other._sampler);
        _skyboxTexture = std::move(other._skyboxTexture);
        _vertexShader = std::move(other._vertexShader);
        _fragmentShader = std::move(other._fragmentShader);
    }

    return *this;
}
