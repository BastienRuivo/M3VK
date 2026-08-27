#pragma once

#include "asset/Vertex.h"
#include "rendering/CommandBuffer.h"
#include <vulkan/vulkan.hpp>

struct VertexState
{
    vk::CullModeFlags CullMode = vk::CullModeFlagBits::eBack;
    vk::VertexInputBindingDescription2EXT BindingDescription = Vertex::GetBindingDescription();
    vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
    std::vector<vk::VertexInputAttributeDescription2EXT> VertexAttributeDescriptions = Vertex::GetAttributeDescription();

    void Bind(const CommandBuffer& cmdBuffer) const
    {
        cmdBuffer.SetVertexInput({&BindingDescription, 1}, VertexAttributeDescriptions);
        cmdBuffer.SetCullMode(CullMode);
    }
};

struct FragmentState
{

    // depth related
    vk::Bool32 depthTest = vk::True;
    // depth compare op
    vk::CompareOp depthCompareOp = vk::CompareOp::eLess;
    vk::Bool32 depthWrite = vk::True;
    vk::Bool32 stencilTest = vk::False;

    vk::Bool32 blending = vk::True;
    // default is opaque so blend one zero
    vk::ColorBlendEquationEXT colorBlendEquation = vk::ColorBlendEquationEXT{}
    .setSrcColorBlendFactor(vk::BlendFactor::eOne)
    .setDstColorBlendFactor(vk::BlendFactor::eZero)
    .setColorBlendOp(vk::BlendOp::eAdd)
    .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
    .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
    .setAlphaBlendOp(vk::BlendOp::eAdd);

    vk::ColorComponentFlags colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    void Bind(const CommandBuffer& cmdBuffer) const
    {
        cmdBuffer.SetDepthCompareOp(depthCompareOp);
        cmdBuffer.SetDepthTest(depthTest);
        cmdBuffer.SetDepthWrite(depthWrite);
        cmdBuffer.SetStencilTest(stencilTest);
        cmdBuffer.SetBlendEnable(blending);
        cmdBuffer.SetBlendEquation(colorBlendEquation);
        cmdBuffer.SetColorWriteMask(colorWriteMask);
    }
};
