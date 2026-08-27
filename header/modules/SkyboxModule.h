#pragma once

#include "handler/Handlers.h"
#include "modules/Module.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GPUImage.h"
#include "rendering/GraphicsImage.h"
#include "rendering/Shaders/ShaderLibrary.h"
class SkyboxModule : public Module
{
public:
    SkyboxModule(ShaderLibrary& shaderLibrary, BindingManager& allocator, vk::CommandPool pool, vk::Queue queue);
    void Execute(const CommandBuffer& cmdBuffer, vk::PipelineLayout layout) const;
    ~SkyboxModule();

    SkyboxModule(SkyboxModule&& other) noexcept;
    SkyboxModule& operator=(SkyboxModule&& other) noexcept;

    SkyboxModule(const SkyboxModule&) = delete;
    SkyboxModule& operator=(const SkyboxModule&) = delete;

    inline const ImageReference SkyboxTexture() const { return _skyboxTexture.Internal(); }

protected:
    VkSamplerHandler _sampler;
    GraphicsImage _skyboxTexture;
    ShaderLibrary::VertexBinding _vertexShader;
    ShaderLibrary::FragmentBinding _fragmentShader;
};
