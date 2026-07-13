#pragma once

#include "rendering/Shaders/ShaderLibrary.h"

class FullscreenDrawDebug
{
public:
    struct TextureConstant
    {
        uint32_t Index;
        uint32_t MipLevel;
    };
    FullscreenDrawDebug(ShaderLibrary& shaderLibrary, const BindingManager& manager);
    ~FullscreenDrawDebug();

    FullscreenDrawDebug(FullscreenDrawDebug&& other) noexcept;
    FullscreenDrawDebug& operator=(FullscreenDrawDebug&& other) noexcept;

    FullscreenDrawDebug(const FullscreenDrawDebug&) = delete;
    FullscreenDrawDebug& operator=(const FullscreenDrawDebug&) = delete;

    void Draw(const CommandBuffer& cmdBuffer, uint32_t textureIndex, VkPipelineLayout layout, uint32_t mipLevel) const;

    ShaderLibrary::VertexBinding VertexBinding;
    ShaderLibrary::FragmentBinding FragmentBinding;
};
