#include "header/HelloTriangleApp.h"
#include "header/CommandBuffer.h"
#include "header/GraphicsBuffer.h"
#include "header/M3VKHelper.h"
#include "header/Shader.h"
#include "header/SwapChain.h"
#include "header/DebugLayer.h"
#include <GLFW/glfw3.h>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/trigonometric.hpp>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void HelloTriangleApp::CreateDescriptorSet()
{
    std::vector<VkDescriptorSetLayout> layouts(MaxFrameInCount, _descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = _descriptorPool;
    allocateInfo.descriptorSetCount = static_cast<uint32_t>(MaxFrameInCount);
    allocateInfo.pSetLayouts = layouts.data();

    _descriptorSets.resize(MaxFrameInCount);
    if(vkAllocateDescriptorSets(_deviceHandler.Get(), &allocateInfo, _descriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor set");
    }

    for(int i = 0; i < MaxFrameInCount; ++i)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = _cameraDataBuffers[i].GetInternal();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(CameraData);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = _descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrite.pImageInfo = nullptr;
        descriptorWrite.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(_deviceHandler.Get(), 1, &descriptorWrite, 0, nullptr);
    }
}

void HelloTriangleApp::CreateDescriptorPool()
{
    VkDescriptorPoolSize poolSize{};
    poolSize.descriptorCount = MaxFrameInCount;
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    VkDescriptorPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.poolSizeCount = 1;
    poolCreateInfo.pPoolSizes = &poolSize;
    poolCreateInfo.maxSets = static_cast<uint32_t>(MaxFrameInCount);

    if(vkCreateDescriptorPool(_deviceHandler.Get(), &poolCreateInfo, nullptr, &_descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("VK Create Descriptor Pool Failed !");
    }
}

void HelloTriangleApp::UpdateCameraData(uint32_t currentFrame)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    CameraData cameraData = {};
    cameraData.localToWorldMatrix = glm::rotate<float>(glm::mat4(1.0f), time * glm::radians<float>(90), glm::vec3(0, 1, 0));
    cameraData.worldToCameraMatrix = glm::lookAt(glm::vec3(2, 2, 2), glm::vec3(0, 0, 0), glm::vec3(0, 1.0, 0));
    cameraData.projectionMatrix = glm::perspective<float>(glm::radians<float>(45),(float)_swapChain->Extent.width / _swapChain->Extent.height, 0.1f,100.0f);
    // it was designed for opengl so flip it
    cameraData.projectionMatrix[1][1] *= -1;

    memcpy(_cameraDataBuffers[currentFrame].GetDataPtr(), &cameraData, sizeof(cameraData));
}

void HelloTriangleApp::CreateCameraDataBuffers()
{
    _cameraDataBuffers.reserve(MaxFrameInCount);

    for(int i = 0; i < MaxFrameInCount; ++i)
    {
        GraphicsBuffer uniform;
        uniform.Create(_physicalDeviceHandler.Get(), _deviceHandler.Get(), 1, sizeof(CameraData), GraphicsBuffer::UNIFORM);
        _cameraDataBuffers.push_back(uniform);
    }
}

void HelloTriangleApp::CreateDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding cameraDataLayoutBindingDescriptor{};
    cameraDataLayoutBindingDescriptor.binding = 0;
    cameraDataLayoutBindingDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraDataLayoutBindingDescriptor.descriptorCount = 1;
    cameraDataLayoutBindingDescriptor.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    cameraDataLayoutBindingDescriptor.pImmutableSamplers = nullptr; // image sampling

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = 1;
    createInfo.pBindings = &cameraDataLayoutBindingDescriptor;

    if(vkCreateDescriptorSetLayout(_deviceHandler.Get(), &createInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
}

static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    HelloTriangleApp* app = reinterpret_cast<HelloTriangleApp*>(glfwGetWindowUserPointer(window));
    app->FramebufferResized();
    app->UpdateWindowSize(width, height);
}

void HelloTriangleApp::UpdateWindowSize(int width, int height)
{
    _window.ResizeWindow(width, height);
}

void HelloTriangleApp::FramebufferResized()
{
    _framebufferResized = true;
}

void HelloTriangleApp::DisposeFramebuffers()
{
    for(size_t i = 0; i < _framebuffers.size(); ++i)
    {
        vkDestroyFramebuffer(_deviceHandler.Get(), _framebuffers[i], nullptr);
    }
}

void HelloTriangleApp::RefreshSwapChain()
{
    vkDeviceWaitIdle(_deviceHandler.Get());

    DisposeFramebuffers();

    // TODO : Swap chain is currently resetted the first frame beacause it is out of date
    _swapChain.reset();
    _swapChain = std::make_unique<SwapChain>(_window, _physicalDeviceHandler.Get(), _deviceHandler.Get(), _windowSurfaceHandler.Get());
    CreateFrameBuffers();
}

void HelloTriangleApp::CreateSyncObject()
{
    _availableImageSemaphores.resize(HelloTriangleApp::MaxFrameInCount);
    _renderFinishedSemaphores.resize(_swapChain->Images.size());
    _waitFences.resize(HelloTriangleApp::MaxFrameInCount);

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // Create the queue in the "Signaled" state to ensure the first frame won't wait eternally for a fence that is not signaled, thus preventing an infinit loop
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for(size_t i = 0; i < HelloTriangleApp::MaxFrameInCount; ++i)
    {

        if(vkCreateSemaphore(_deviceHandler.Get(), &semaphoreCreateInfo, nullptr, &_availableImageSemaphores[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Can't create image available semaphore");
        }

        if(vkCreateFence(_deviceHandler.Get(), &fenceCreateInfo, nullptr, &_waitFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Can't create fence");
        }
    }

    for(size_t i = 0; i < _swapChain->Images.size(); ++i)
    {
        if(vkCreateSemaphore(_deviceHandler.Get(), &semaphoreCreateInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Can't create render finished semaphore");
        }
    }
}

void HelloTriangleApp::DrawFrame()
{
    vkWaitForFences(_deviceHandler.Get(), 1, &_waitFences[_currentFrame], VK_TRUE, UINT64_MAX);

    // Acquire image to draw on
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_deviceHandler.Get(), _swapChain->_internal, UINT64_MAX, _availableImageSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);

    if(result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RefreshSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Only reset the fence if we are submitting work
    vkResetFences(_deviceHandler.Get(), 1, &_waitFences[_currentFrame]);

    UpdateCameraData(_currentFrame);

    _commandBuffers[_currentFrame].Reset();
    RecordCommandBuffer(_commandBuffers[_currentFrame], _currentFrame, imageIndex);

    // stackallocs that can be cached.
    VkSemaphore wait[] = {_availableImageSemaphores[_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphore[] = {_renderFinishedSemaphores[imageIndex]};

    _commandBuffers[_currentFrame].Submit(wait, 1, waitStages, signalSemaphore, 1, _waitFences[_currentFrame]);

    // actually present the frame
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphore;

    VkSwapchainKHR swapChains[] = {_swapChain->_internal};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(_graphicsQueueHandler.Get(), &presentInfo);

    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized)
    {
        _framebufferResized = false;
        RefreshSwapChain();
    }
    else if(result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    _currentFrame = (_currentFrame + 1) % HelloTriangleApp::MaxFrameInCount;
}

void HelloTriangleApp::RecordCommandBuffer(CommandBuffer cmdBuffer, uint32_t currentFrame, uint32_t imageIndex)
{
    cmdBuffer.Begin();
    {
        cmdBuffer.BeginRenderPass(_renderPassHandler.Get(), _framebuffers[imageIndex], {0, 0, 0, 0}, _swapChain->Extent);
        {
            cmdBuffer.BindPipeline(_graphicsPipeline, VK_PIPELINE_BIND_POINT_GRAPHICS);
            cmdBuffer.SetViewport(_swapChain->Extent.width, _swapChain->Extent.height);
            cmdBuffer.SetScissor(0, 0, _swapChain->Extent.width, _swapChain->Extent.height);
            cmdBuffer.BindBuffer(_vertexBuffer);
            cmdBuffer.BindBuffer(_indexBuffer);
            cmdBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, _descriptorSets[currentFrame]);
            cmdBuffer.DrawIndexed(static_cast<uint32_t>(_indices.size()));
        }
        cmdBuffer.EndRenderPass();
    }
    cmdBuffer.End();
}

void HelloTriangleApp::CreateCommandBuffers()
{
    _commandBuffers.reserve(HelloTriangleApp::MaxFrameInCount);

    // Allocate in batch ?
    for (int i = 0; i < HelloTriangleApp::MaxFrameInCount; ++i)
    {
        CommandBuffer cmdBuffer(_deviceHandler.Get(), _graphicsCommandPool, _graphicsQueueHandler.Get());
        _commandBuffers.emplace_back(cmdBuffer);
    }
}

void HelloTriangleApp::CreatCommandPool()
{
    M3VKHelper::QueueFamilyIds queueFamilyId = M3VKHelper::QueryQueueFamilies(_physicalDeviceHandler.Get(), _windowSurfaceHandler.Get());

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyId.Graphics.value();

    if(vkCreateCommandPool(_deviceHandler.Get(), &poolInfo, nullptr, &_graphicsCommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool !");
    }


}

void HelloTriangleApp::CreateFrameBuffers()
{
    _framebuffers.resize(_swapChain->ImageViews.size());

    for(size_t i = 0; i < _swapChain->ImageViews.size(); ++i)
    {
        VkImageView attachments[] = {
            _swapChain->ImageViews[i]
        };

        VkFramebufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = _renderPassHandler.Get();
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = attachments;
        createInfo.width = _swapChain->Extent.width;
        createInfo.height = _swapChain->Extent.height;
        createInfo.layers = 1;

        if(vkCreateFramebuffer(_deviceHandler.Get(), &createInfo, nullptr, &_framebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer !");
        }
    }
}

void HelloTriangleApp::CreateGraphicsPipeline()
{
    Shader shader(_deviceHandler.Get());

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
    viewport.width = _swapChain->Extent.width;
    viewport.height = _swapChain->Extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Region to rasterize pixels on
    VkRect2D scissors{};
    scissors.extent = _swapChain->Extent;
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

    // Uniforms (empty for now)
    VkPipelineLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = 1;
    layoutCreateInfo.pSetLayouts = &_descriptorSetLayout;
    layoutCreateInfo.pushConstantRangeCount = 0; // Optional
    layoutCreateInfo.pPushConstantRanges = nullptr; // Optional

    if(vkCreatePipelineLayout(_deviceHandler.Get(), &layoutCreateInfo, nullptr, &_pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create VK Layout !");
    }

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

    pipelineCreateInfo.layout = _pipelineLayout;
    pipelineCreateInfo.renderPass = _renderPassHandler.Get();
    pipelineCreateInfo.subpass = 0;

    // can be use to switch between parent pipeline (less expensive theorically if simillar)
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    if(vkCreateGraphicsPipelines(_deviceHandler.Get(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &_graphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline !");
    }
}

HelloTriangleApp::HelloTriangleApp() :
    _renderPassHandler(_deviceHandler.Get(), _swapChain->ImageFormat),
    _swapChain(std::make_unique<SwapChain>(_window, _physicalDeviceHandler.Get(), _deviceHandler.Get(), _windowSurfaceHandler.Get())),
    _presentQueueHandler(_deviceHandler.Get(), _physicalDeviceHandler.QueueFamilyIds, VkQueueHandler::Present),
    _graphicsQueueHandler(_deviceHandler.Get(), _physicalDeviceHandler.QueueFamilyIds, VkQueueHandler::Graphics),
    _deviceHandler(_physicalDeviceHandler, _windowSurfaceHandler.Get(), _deviceExtensions),
    _physicalDeviceHandler(_instanceHandler.Get(), _windowSurfaceHandler.Get(), _deviceExtensions),
    _windowSurfaceHandler(_instanceHandler.Get(), _window.Get()),
    _vkDebugLayer(_instanceHandler.Get()),
    _instanceHandler(),
    _window(1920, 1080, "Window", this, FramebufferResizeCallback)
{
    DebugLayer::Log(DebugLayer::LogType::CREATE, "HelloTriangleApp Creation !");

    CreateDescriptorSetLayout();
    CreateGraphicsPipeline();
    CreateFrameBuffers();
    CreatCommandPool();
    CreateCameraDataBuffers();
    CreateDescriptorPool();
    CreateDescriptorSet();

    // init data
    _indexBuffer.Create(_physicalDeviceHandler.Get(), _deviceHandler.Get(), _indices.size(), sizeof(_indices[0]), GraphicsBuffer::BufferType::INDEX);
    _indexBuffer.CopyToBuffer(_physicalDeviceHandler.Get(), _deviceHandler.Get(), _graphicsQueueHandler.Get(), _graphicsCommandPool, (void*)_indices.data(), _indexBuffer.GetSize());

    _vertexBuffer.Create(_physicalDeviceHandler.Get(), _deviceHandler.Get(), _vertices.size(), sizeof(_vertices[0]), GraphicsBuffer::BufferType::VERTEX);
    _vertexBuffer.CopyToBuffer(_physicalDeviceHandler.Get(), _deviceHandler.Get(), _graphicsQueueHandler.Get(), _graphicsCommandPool, (void*)_vertices.data(), _vertexBuffer.GetSize());

    CreateCommandBuffers();
    CreateSyncObject();
}

void HelloTriangleApp::MainLoop()
{
    while (!_window.ShouldClose())
    {
        _window.ProcessEvent();
        DrawFrame();
    }

    vkDeviceWaitIdle(_deviceHandler.Get());
}

HelloTriangleApp::~HelloTriangleApp()
{
    for(size_t i = 0; i < HelloTriangleApp::MaxFrameInCount; ++i)
    {
        vkDestroySemaphore(_deviceHandler.Get(), _availableImageSemaphores[i], nullptr);
        vkDestroyFence(_deviceHandler.Get(), _waitFences[i], nullptr);
    }

    for(size_t i = 0; i < _swapChain->Images.size(); ++i)
    {
        vkDestroySemaphore(_deviceHandler.Get(), _renderFinishedSemaphores[i], nullptr);
    }

    vkDestroyCommandPool(_deviceHandler.Get(), _graphicsCommandPool, nullptr);
    DisposeFramebuffers();

    _vertexBuffer.DisposeBuffer(_deviceHandler.Get());
    _indexBuffer.DisposeBuffer(_deviceHandler.Get());

    for(int i = 0; i < _cameraDataBuffers.size(); ++i)
    {
        _cameraDataBuffers[i].DisposeBuffer(_deviceHandler.Get());
    }

    vkDestroyDescriptorPool(_deviceHandler.Get(), _descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(_deviceHandler.Get(), _descriptorSetLayout, nullptr);

    vkDestroyPipeline(_deviceHandler.Get(), _graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(_deviceHandler.Get(), _pipelineLayout, nullptr);
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "HelloTriangleApp Destroyed !");
}

void HelloTriangleApp::Run()
{
    MainLoop();
}
