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

    DrawModule(DrawModule&& other) noexcept = default;
    DrawModule& operator=(DrawModule&& other) noexcept = default;

    DrawModule(const DrawModule&) = delete;
    DrawModule& operator=(const DrawModule&) = delete;

    void Execute(const CommandBuffer& cmdBuffer, vk::PipelineLayout layout, const GraphicsBuffer& indirectBuffer, uint32_t drawOffset, uint32_t drawCount) const;
    void Execute(const CommandBuffer& cmdBuffer, vk::PipelineLayout layout, const VertexState& vertexState, const FragmentState& fragmentState, const GraphicsBuffer& indirectBuffer, uint32_t drawOffset, uint32_t drawCount) const;
    ShaderLibrary::VertexBinding VertexBinding;
    ShaderLibrary::FragmentBinding FragmentBinding;
};
