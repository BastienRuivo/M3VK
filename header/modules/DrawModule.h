#pragma once

#include "modules/Module.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include <cstdint>

class DrawModule : public Module
{
public:
    DrawModule(ShaderLibrary::VertexBinding vertexBinding, ShaderLibrary::FragmentBinding fragmentBinding);
    ~DrawModule();

    void Execute(const CommandBuffer& cmdBuffer, VkPipelineLayout layout, const GraphicsBuffer& indirectBuffer, uint32_t drawOffset, uint32_t drawCount, bool wireframe) const;
    ShaderLibrary::VertexBinding VertexBinding;
    ShaderLibrary::FragmentBinding FragmentBinding;
};
