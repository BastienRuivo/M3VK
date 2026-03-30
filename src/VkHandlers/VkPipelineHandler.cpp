#include "header/VkHandlers/VkPipelineHandler.h"
#include "header/Shader.h"
#include "header/Vertex.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

VkPipelineHandler::VkPipelineHandler(const VkExtent2D& appExtent, VkDevice device, VkPipelineLayout pipelineLayout, VkRenderPass renderPass)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkPipelineHandler Creation !");
#endif

    _device = device;

    Shader shader(_device);

    VkPipelineShaderStageCreateInfo shadersStagesCreateInfo[2] = {
        {},
        {}
    };

    shadersStagesCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shadersStagesCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shadersStagesCreateInfo[0].module = shader.VertexShader;
    shadersStagesCreateInfo[0].pName = "main";

    shadersStagesCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shadersStagesCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shadersStagesCreateInfo[1].module = shader.FragmentShader;
    shadersStagesCreateInfo[1].pName = "main";

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo pipelineStateCreateInfo{};
    pipelineStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipelineStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    pipelineStateCreateInfo.pDynamicStates = dynamicStates.data();

    VkVertexInputBindingDescription bindingDescription = Vertex::GetBindingDescription();
    std::array<VkVertexInputAttributeDescription, 2> attributeDescription = Vertex::GetAttributeDescription();


    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
    vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescription.data();

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    // For later use (When i will choose if i still want to make a cube world like thingy) maybe i can try strip ?
    // like in opengl, this turn 4 index like 1234 into 2 triangle (123, 324) and this can save me some place (bandwith <3)
    // theres also some element buffer shit to setup here later
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = false;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = appExtent.width;
    viewport.height = appExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Region to rasterize pixels on
    VkRect2D scissors{};
    scissors.extent = appExtent;
    scissors.offset.x = 0;
    scissors.offset.y = 0;

    VkPipelineViewportStateCreateInfo viewportCreateInfo{};
    viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportCreateInfo.viewportCount = 1;
    viewportCreateInfo.scissorCount = 1;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizeCreateInfo{};
    rasterizeCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // if true, clamp near and far object to the planes instead of discarding them
    rasterizeCreateInfo.depthClampEnable = false;
    // disable geometry from going to the rasterizer stage ??
    rasterizeCreateInfo.rasterizerDiscardEnable = false;
    // VK_POLYGON_MODE_LINE for wireframe later maybe ? or this can be another feature to enable, check later
    rasterizeCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizeCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizeCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; // because of y *=
    rasterizeCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizeCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizeCreateInfo.depthBiasClamp = 0.0f;
    rasterizeCreateInfo.depthBiasSlopeFactor = 0.0f;
    rasterizeCreateInfo.lineWidth = 1.0f;

    // MSAA, disabled for now
    VkPipelineMultisampleStateCreateInfo msaaCreateInfo{};
    msaaCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaaCreateInfo.sampleShadingEnable = VK_FALSE;
    msaaCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    msaaCreateInfo.minSampleShading = 1.0f;
    msaaCreateInfo.pSampleMask = nullptr;
    msaaCreateInfo.alphaToCoverageEnable = VK_FALSE;
    msaaCreateInfo.alphaToOneEnable = VK_FALSE;

    // Here depth later

    // Blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;

    // Graphics pipeline
    pipelineCreateInfo.pStages = shadersStagesCreateInfo;
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizeCreateInfo;
    pipelineCreateInfo.pMultisampleState = &msaaCreateInfo;
    pipelineCreateInfo.pDepthStencilState = nullptr;
    pipelineCreateInfo.pColorBlendState = &colorBlending;
    pipelineCreateInfo.pDynamicState = &pipelineStateCreateInfo;

    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;

    // can be use to switch between parent pipeline (less expensive theorically if simillar)
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    if(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline !");
    }
}
VkPipelineHandler::~VkPipelineHandler()
{
    if(_internal == VK_NULL_HANDLE) return;
    vkDestroyPipeline(_device, _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkPipelineHandler Destroyed !");
#endif
}

VkPipelineHandler::VkPipelineHandler(VkPipelineHandler && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkPipelineHandler Move Creation !");
#endif

    _internal = other._internal;
    _device = other._device;
    other._internal = VK_NULL_HANDLE;
}

VkPipelineHandler& VkPipelineHandler::operator=(VkPipelineHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        _device = other._device;
        other._internal = VK_NULL_HANDLE;
    }

    return *this;
}
