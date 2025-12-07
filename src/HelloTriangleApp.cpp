#include "header/HelloTriangleApp.h"
#include "header/M3VKHelper.h"
#include "header/Shader.h"
#include <GLFW/glfw3.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

void HelloTriangleApp::CreateVertexBuffer()
{
    VkDeviceSize size = _vertices.size() * sizeof(_vertices[0]);

    VkBuffer staggingBuffer;
    VkDeviceMemory staggingMemory;
    M3VKHelper::CreateBuffer(_physicalDevice,
        _device,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staggingBuffer, staggingMemory);

    M3VKHelper::CopyToBuffer(_device, (void*)_vertices.data(), staggingMemory, 0, size);

    M3VKHelper::CreateBuffer(_physicalDevice,
        _device,
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        _vertexBuffer, _vertexBufferMemory);

    M3VKHelper::CopyBufferToBuffer(_device, _graphicsQueue, _graphicsCommandPool, staggingBuffer, 0, _vertexBuffer, 0, size);

    vkDestroyBuffer(_device, staggingBuffer, nullptr);
    vkFreeMemory(_device, staggingMemory, nullptr);
}

static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    HelloTriangleApp* app = reinterpret_cast<HelloTriangleApp*>(glfwGetWindowUserPointer(window));
    app->FramebufferResized();
}

void HelloTriangleApp::FramebufferResized()
{
    _framebufferResized = true;
}

void HelloTriangleApp::DisposeSwapChain()
{
    for(size_t i = 0; i < _framebuffers.size(); ++i)
    {
        vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);
    }
    _swapChain.Dipose(_device);
}

void HelloTriangleApp::RefreshSwapChain()
{
    vkDeviceWaitIdle(_device);

    DisposeSwapChain();

    _swapChain.Create(_pWindow, _physicalDevice, _device, _windowSurface);
    CreateFrameBuffers();
}

void HelloTriangleApp::CreateSyncObject()
{
    _availableImageSemaphores.resize(HelloTriangleApp::MaxFrameInCount);
    _renderFinishedSemaphores.resize(_swapChain.Images.size());
    _waitFences.resize(HelloTriangleApp::MaxFrameInCount);

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // Create the queue in the "Signaled" state to ensure the first frame won't wait eternally for a fence that is not signaled, thus preventing an infinit loop
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for(size_t i = 0; i < HelloTriangleApp::MaxFrameInCount; ++i)
    {

        if(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_availableImageSemaphores[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Can't create image available semaphore");
        }

        if(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_waitFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Can't create fence");
        }
    }

    for(size_t i = 0; i < _swapChain.Images.size(); ++i)
    {
        if(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Can't create render finished semaphore");
        }
    }
}

void HelloTriangleApp::DrawFrame()
{
    vkWaitForFences(_device, 1, &_waitFences[_currentFrame], VK_TRUE, UINT64_MAX);

    // Acquire image to draw on
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_device, _swapChain.Internal, UINT64_MAX, _availableImageSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);

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
    vkResetFences(_device, 1, &_waitFences[_currentFrame]);

    vkResetCommandBuffer(_commandBuffers[_currentFrame], 0);
    RecordCommandBuffer(_commandBuffers[_currentFrame], imageIndex);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // stackallocs that can be cached.
    VkSemaphore wait[] = {_availableImageSemaphores[_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = wait;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffers[_currentFrame];

    VkSemaphore signalSemaphore[] = {_renderFinishedSemaphores[imageIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphore;

    if(vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _waitFences[_currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command");
    }

    // actually present the frame
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphore;

    VkSwapchainKHR swapChains[] = {_swapChain.Internal};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(_graphicsQueue, &presentInfo);

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

void HelloTriangleApp::RecordCommandBuffer(VkCommandBuffer cmdBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Tells how we're using command buffer (record each send, buffer used in a render pass...)
    beginInfo.pInheritanceInfo = nullptr; // state info when called by a primary command buffer when it's a secondary one

    if(vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to begin command buffer !");
    }

    VkRenderPassBeginInfo rpBeginInfo{};
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = _renderPass;
    rpBeginInfo.framebuffer = _framebuffers[imageIndex];
    rpBeginInfo.renderArea.offset = {0, 0};
    rpBeginInfo.renderArea.extent =_swapChain.Extent;

    VkClearValue clearValue{};
    clearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};

    rpBeginInfo.clearValueCount = 1;
    rpBeginInfo.pClearValues = &clearValue;

    // inline tells everything is embedded in the primary cmdBuffer and no secondary will be used
    vkCmdBeginRenderPass(cmdBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = _swapChain.Extent.width;
    viewport.height = _swapChain.Extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

    VkRect2D scissors{};
    scissors.offset = {0, 0};
    scissors.extent = _swapChain.Extent;

    vkCmdSetScissor(cmdBuffer, 0, 1, &scissors);

    VkBuffer vertexBuffers[] = {_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdDraw(cmdBuffer, static_cast<uint32_t>(_vertices.size()), 1, 0, 0);

    vkCmdEndRenderPass(cmdBuffer);

    if(vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to record command buffer");
    }
}

void HelloTriangleApp::CreateCommandBuffers()
{
    _commandBuffers.resize(HelloTriangleApp::MaxFrameInCount);

    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = _graphicsCommandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());

    if(vkAllocateCommandBuffers(_device, &allocateInfo, _commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command buffer !");
    }
}

void HelloTriangleApp::CreatCommandPool()
{
    M3VKHelper::QueueFamilyId queueFamilyId = M3VKHelper::QueryQueueFamilies(_physicalDevice, _windowSurface);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyId.Graphics.value();

    if(vkCreateCommandPool(_device, &poolInfo, nullptr, &_graphicsCommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool !");
    }


}

void HelloTriangleApp::CreateFrameBuffers()
{
    _framebuffers.resize(_swapChain.ImageViews.size());

    for(size_t i = 0; i < _swapChain.ImageViews.size(); ++i)
    {
        VkImageView attachments[] = {
            _swapChain.ImageViews[i]
        };

        VkFramebufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = _renderPass;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = attachments;
        createInfo.width = _swapChain.Extent.width;
        createInfo.height = _swapChain.Extent.height;
        createInfo.layers = 1;

        if(vkCreateFramebuffer(_device, &createInfo, nullptr, &_framebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer !");
        }
    }
}

void HelloTriangleApp::CreateRenderPass()
{
    VkAttachmentDescription colorAttachment =  {};
    colorAttachment.format = _swapChain.ImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // clear at new frame, load to keep and dont care to undefined
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // STORE preserve the data, DONT_CARE otherwise
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // only rendering a triangle, no use of stencil or depth atm
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // Images can have layout suitable for different op
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    // attachment index, only one so 0
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint =VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo rpCreateInfo{};
    rpCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpCreateInfo.attachmentCount = 1;
    rpCreateInfo.pAttachments = &colorAttachment;
    rpCreateInfo.subpassCount = 1;
    rpCreateInfo.pSubpasses = &subpass;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // the passe before or after depending if it's used in src or dst
    dependency.dstSubpass = 0; // our pass

    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;

    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    rpCreateInfo.dependencyCount = 1;
    rpCreateInfo.pDependencies = &dependency;

    if(vkCreateRenderPass(_device, &rpCreateInfo, nullptr, &_renderPass)  != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create render pass");
    }
}

void HelloTriangleApp::CreateGraphicsPipeline()
{
    Shader shader;
    shader.Create(_device);

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
    viewport.width = _swapChain.Extent.width;
    viewport.height = _swapChain.Extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Region to rasterize pixels on
    VkRect2D scissors{};
    scissors.extent = _swapChain.Extent;
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
    rasterizeCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
    layoutCreateInfo.setLayoutCount = 0;
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = 0; // Optional
    layoutCreateInfo.pSetLayouts = nullptr; // Optional
    layoutCreateInfo.pushConstantRangeCount = 0; // Optional
    layoutCreateInfo.pPushConstantRanges = nullptr; // Optional

    if(vkCreatePipelineLayout(_device, &layoutCreateInfo, nullptr, &_pipelineLayout) != VK_SUCCESS)
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
    pipelineCreateInfo.renderPass = _renderPass;
    pipelineCreateInfo.subpass = 0;

    // can be use to switch between parent pipeline (less expensive theorically if simillar)
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    if(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &_graphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline !");
    }

    // Disposal
    shader.Dispose(_device);
}

bool HelloTriangleApp::CheckDeviceExtensionSupport(const VkPhysicalDevice& device) const
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> properties(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, properties.data());

    for(const auto& extension : _deviceExtensions)
    {
        bool foundExtension = false;
        for(const VkExtensionProperties& property : properties)
        {
            if(strcmp(extension, property.extensionName) == 0)
            {
                foundExtension = true;
                break;
            }
        }
        if(!foundExtension)
        {
            if(_vkDebugLayer.Enabled)
            {
                _vkDebugLayer.LogError((std::string("Extension not supported : ") + std::string(extension)).c_str());
            }
            return false;
        }
    }

    return true;
}

void HelloTriangleApp::CreateWindowSurface()
{
    if(glfwCreateWindowSurface(_instance, _pWindow, nullptr, &_windowSurface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create windows surface !");
    }
}

void HelloTriangleApp::CreateLogicalDevice()
{
    M3VKHelper::QueueFamilyId queueFamilyId = M3VKHelper::QueryQueueFamilies(_physicalDevice, _windowSurface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueIds =
    {
        queueFamilyId.Graphics.value(),
        queueFamilyId.Present.value(),
        //queueFamilyId.Copy.value()
    };

    float queuePriority = 1.0f;
    for(uint32_t queueId : uniqueQueueIds)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueId;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(_deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = _deviceExtensions.data();

    if(_vkDebugLayer.Enabled)
    {
        deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(_vkDebugLayer.validationLayer.size());
        deviceCreateInfo.ppEnabledLayerNames = _vkDebugLayer.validationLayer.data();
    }
    else
    {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    VkResult deviceCreation = vkCreateDevice(_physicalDevice, &deviceCreateInfo, nullptr, &_device);
    if(deviceCreation != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create VK Logical Device !");
    }

    vkGetDeviceQueue(_device, queueFamilyId.Graphics.value(), 0, &_graphicsQueue);
    vkGetDeviceQueue(_device, queueFamilyId.Present.value(), 0, &_presentQueue);
    // use it
    //vkGetDeviceQueue(_device, queueFamilyId.Copy.value(), 0, &_copyQueue);
}

int HelloTriangleApp::ScoreDeviceSuitability(const VkPhysicalDevice& device) const
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // support of addtionnal feature (texture compression, 64bit double, multi viewport rendering)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int score = 0;


    M3VKHelper::QueueFamilyId ids = M3VKHelper::QueryQueueFamilies(device, _windowSurface);

    bool areAllRequiredExtensionsSupported = CheckDeviceExtensionSupport(device);

    M3VKHelper::SwapChainSupportDetails swapChainDetails = M3VKHelper::QuerySwapChainSupportDetail(device, _windowSurface);

    // Mandatory feature, if any return 0 and this will be the only way to have 0 score meaning there's no suitable GPU
    if(!M3VKHelper::QueueFamilyId::AreAllQueueAvailable(ids)
        || !areAllRequiredExtensionsSupported
        || !swapChainDetails.CheckSwapChainSupportAdequate())
    {
        return 0;
    }

    // Else we try to find the best available GPU for our criteria
    switch (deviceProperties.deviceType)
    {
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: score += 600; break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: score += 800; break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: score += 1000; break;
        default: break;
    }

    return score;
}

void HelloTriangleApp::PickPhysicalDevice()
{
    _physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

    if(deviceCount == 0)
    {
        throw std::runtime_error("Failed to find a Vulkan compatible GPU on this device");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

    int bestScore = 0;
    for(const VkPhysicalDevice& device : devices)
    {
        int score = ScoreDeviceSuitability(device);
        if(score > bestScore)
        {
            bestScore = score;
            _physicalDevice = device;
        }
    }

    if(_physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable GPU on this device");
    }
}

void HelloTriangleApp::InitWindow()
{
    glfwInit();
    // GLFW is made for GL (No shit) so create need an empty API for vk
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // TODO : Handle RESIZABLE window later
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    _pWindow = glfwCreateWindow(HelloTriangleApp::WindowWidth, HelloTriangleApp::WindowHeight, "H3ll0 Tri4ngl3", nullptr, nullptr);
    glfwSetWindowUserPointer(_pWindow, this);
    glfwSetFramebufferSizeCallback(_pWindow, FramebufferResizeCallback);
}

void HelloTriangleApp::InitVulkan()
{
    CreateVKInstance();
    _vkDebugLayer.Create(_instance);
    CreateWindowSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    _swapChain.Create(_pWindow, _physicalDevice, _device, _windowSurface);
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFrameBuffers();
    CreatCommandPool();
    CreateVertexBuffer();
    CreateCommandBuffers();
    CreateSyncObject();
}

void HelloTriangleApp::CreateVKInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "M3VK";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_4;

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, supportedExtensions.data());

    if(_vkDebugLayer.Enabled)
    {
        _vkDebugLayer.LogInfo("List of actives VK Extensions");
        for(const VkExtensionProperties& extension : supportedExtensions)
        {
            _vkDebugLayer.LogInfo(std::string("\t - ") + extension.extensionName);
        }
    }


    std::vector<const char *> requiredExtensions = GetRequiredExtensions();
    for(int i = 0; i < requiredExtensions.size(); ++i)
    {
        bool isPresent = false;
        for(const VkExtensionProperties& extension : supportedExtensions)
        {
            if(strcmp(requiredExtensions[i], extension.extensionName) == 0)
            {
                isPresent = true;
                break;
            }
        }

        if(!isPresent)
        {
            throw std::runtime_error("Vulkan Extension" + std::string(requiredExtensions[i]) + " Is not supported but required for GLFW");
        }
    }

    if(!_vkDebugLayer.CheckValidationLayerSupport())
    {
        throw std::runtime_error("Validation layer requested but not available !");
    }

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    // ensure it is not destroyed before vkCreateInstance
    VkDebugUtilsMessengerCreateInfoEXT debugInfoCreate;
    _vkDebugLayer.SetupCreateInfo(createInfo, debugInfoCreate);

    VkResult result = vkCreateInstance(&createInfo, nullptr, &_instance);
    if(result != VK_SUCCESS)
    {
        if(result == VK_ERROR_LAYER_NOT_PRESENT)
        {
            throw std::runtime_error("Error : A VK Layer is not present on computer");
        }
        else
        {
            throw std::runtime_error("Failed to create VK_Instance");
        }
    }
}

std::vector<const char *> HelloTriangleApp::GetRequiredExtensions() const
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if(_vkDebugLayer.Enabled)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void HelloTriangleApp::MainLoop()
{
    while (!glfwWindowShouldClose(_pWindow))
    {
        glfwPollEvents();
        DrawFrame();
    }

    vkDeviceWaitIdle(_device);
}

void HelloTriangleApp::DisposeWindow()
{
    glfwDestroyWindow(_pWindow);
    glfwTerminate();
}

void HelloTriangleApp::Dispose()
{
    for(size_t i = 0; i < HelloTriangleApp::MaxFrameInCount; ++i)
    {
        vkDestroySemaphore(_device, _availableImageSemaphores[i], nullptr);
        vkDestroyFence(_device, _waitFences[i], nullptr);
    }

    for(size_t i = 0; i < _swapChain.Images.size(); ++i)
    {
        vkDestroySemaphore(_device, _renderFinishedSemaphores[i], nullptr);
    }

    vkDestroyCommandPool(_device, _graphicsCommandPool, nullptr);
    DisposeSwapChain();
    vkDestroyBuffer(_device, _vertexBuffer, nullptr);
    vkFreeMemory(_device, _vertexBufferMemory, nullptr);
    vkDestroyPipeline(_device, _graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
    vkDestroyRenderPass(_device, _renderPass, nullptr);
    vkDestroyDevice(_device, nullptr);
    _vkDebugLayer.Dispose(_instance);
    vkDestroySurfaceKHR(_instance, _windowSurface, nullptr);
    vkDestroyInstance(_instance, nullptr);
    DisposeWindow();
}

void HelloTriangleApp::Run()
{
    InitWindow();
    InitVulkan();

    MainLoop();
    Dispose();
}
