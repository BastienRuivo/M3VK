#pragma once

#include "rendering/GraphicsBuffer.h"
#include "rendering/Shaders/ShaderLibrary.h"
class DrawModule
{
public:
    DrawModule(ShaderLibrary::VertexBinding vertexBinding, ShaderLibrary::FragmentBinding fragmentBinding);
    ~DrawModule();

    void Execute(const CommandBuffer& cmdBuffer, VkPipelineLayout layout, const GraphicsBuffer& indirectBuffer, uint32_t drawCount, bool wireframe) const;

private:
    ShaderLibrary::VertexBinding _vertexBinding;
    ShaderLibrary::FragmentBinding _fragmentBinding;
};
