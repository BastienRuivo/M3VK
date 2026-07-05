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
    SkyboxModule(ShaderLibrary& shaderLibrary, BindingManager& allocator, VkCommandPool pool, VkQueue queue);
    void Execute(const CommandBuffer& cmdBuffer, VkPipelineLayout layout) const;
    ~SkyboxModule();

    inline const ImageReference SkyboxTexture() const { return _skyboxTexture.Internal(); }

protected:
    VkSamplerHandler _sampler;
    GraphicsImage _skyboxTexture;
    ShaderLibrary::VertexBinding _vertexShader;
    ShaderLibrary::FragmentBinding _fragmentShader;
};
