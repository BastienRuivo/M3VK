#pragma once

#include "asset/Vertex.h"
#include "rendering/CommandBuffer.h"
#include <vulkan/vulkan_core.h>

struct ShaderState
{
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    VkVertexInputBindingDescription2EXT bindingDescription = Vertex::GetBindingDescription();
    std::vector<VkVertexInputAttributeDescription2EXT> vertexAttributeDescriptions = Vertex::GetAttributeDescription();

    // depth related
    VkBool32 depthTest = VK_TRUE;
    // depth compare op
    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
    VkBool32 depthWrite = VK_TRUE;
    VkBool32 stencilTest = VK_FALSE;

    VkBool32 blending = VK_TRUE;
    // default is opaque so blend one zero
    VkColorBlendEquationEXT colorBlendEquation =
    {
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD
    };

    VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    void Bind(const CommandBuffer& cmdBuffer) const
    {
        cmdBuffer.SetVertexInput(1, &bindingDescription, static_cast<uint32_t>(vertexAttributeDescriptions.size()), vertexAttributeDescriptions.data());
        cmdBuffer.SetCullMode(cullMode);
        cmdBuffer.SetDepthCompareOp(depthCompareOp);
        cmdBuffer.SetDepthTest(depthTest);
        cmdBuffer.SetDepthWrite(depthWrite);
        cmdBuffer.SetStencilTest(stencilTest);
        cmdBuffer.SetBlendEnable(blending);
        cmdBuffer.SetBlendEquation(colorBlendEquation);
        cmdBuffer.SetColorWriteMask(colorWriteMask);
    }
};
