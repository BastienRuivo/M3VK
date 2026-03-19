#include "header/HelloTriangleApp.h"
#include "header/CommandBuffer.h"
#include "header/GraphicsBuffer.h"
#include "header/SwapChain.h"
#include "header/DebugLayer.h"
#include "header/VkHandlers/VkFramebufferHandler.h"
#include <GLFW/glfw3.h>
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
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void HelloTriangleApp::CreateDescriptorSet()
{
    std::vector<VkDescriptorSetLayout> layouts(MaxFrameInCount, _descriptorSetLayoutHandler.Get());
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
        _cameraDataBuffers.emplace_back(_physicalDeviceHandler, _deviceHandler.Get(), 1, sizeof(CameraData), GraphicsBuffer::UNIFORM);
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

void HelloTriangleApp::RefreshSwapChain()
{
    vkDeviceWaitIdle(_deviceHandler.Get());

    // TODO : Swap chain is currently resetted the first frame beacause it is out of date
    _swapChain.reset();
    _swapChain = std::make_unique<SwapChain>(_window, _physicalDeviceHandler.Get(), _deviceHandler.Get(), _windowSurfaceHandler.Get());

    _framebuffersHandler.clear();
    _framebuffersHandler = CreateFrameBuffers();
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
        cmdBuffer.BeginRenderPass(_renderPassHandler.Get(), _framebuffersHandler[imageIndex].Get(), {0, 0, 0, 0}, _swapChain->Extent);
        {
            cmdBuffer.BindPipeline(_graphicsPipelineHandler.Get(), VK_PIPELINE_BIND_POINT_GRAPHICS);
            cmdBuffer.SetViewport(_swapChain->Extent.width, _swapChain->Extent.height);
            cmdBuffer.SetScissor(0, 0, _swapChain->Extent.width, _swapChain->Extent.height);
            cmdBuffer.BindBuffer(_vertexBuffer);
            cmdBuffer.BindBuffer(_indexBuffer);
            cmdBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayoutHandler.Get(), _descriptorSets[currentFrame]);
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
        CommandBuffer cmdBuffer(_deviceHandler.Get(), _graphicsCommandPoolHandler.Get(), _graphicsQueueHandler.Get());
        _commandBuffers.emplace_back(cmdBuffer);
    }
}

std::vector<VkFramebufferHandler> HelloTriangleApp::CreateFrameBuffers()
{
    std::vector<VkFramebufferHandler> framebuffers;
    framebuffers.reserve(_swapChain->ImageViews.size());

    for(size_t i = 0; i < _swapChain->ImageViews.size(); ++i)
    {
        std::vector<VkImageView> attachments = {
            _swapChain->ImageViews[i]
        };
        // emplace back to avoid a temp obejct
        framebuffers.emplace_back(_deviceHandler.Get(), _renderPassHandler.Get(), _swapChain->Extent, attachments);
    }
    return framebuffers;
}

HelloTriangleApp::HelloTriangleApp() :
    _vertexBuffer(_physicalDeviceHandler, _deviceHandler.Get(), _vertices.size(), sizeof(_vertices[0]), GraphicsBuffer::BufferType::VERTEX),
    _indexBuffer(_physicalDeviceHandler, _deviceHandler.Get(), _indices.size(), sizeof(_indices[0]), GraphicsBuffer::BufferType::INDEX),
    _graphicsCommandPoolHandler(_deviceHandler.Get(), _physicalDeviceHandler.QueueFamilyIds),
    _framebuffersHandler(CreateFrameBuffers()),
    _graphicsPipelineHandler(_swapChain->Extent, _deviceHandler.Get(), _pipelineLayoutHandler.Get(), _renderPassHandler.Get()),
    _pipelineLayoutHandler(_deviceHandler.Get(), _descriptorSetLayoutHandler.Get()),
    _descriptorSetLayoutHandler(_deviceHandler.Get()),
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
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "HelloTriangleApp Creation !");
#endif

    CreateCameraDataBuffers();
    CreateDescriptorPool();
    CreateDescriptorSet();

    // init data
    _indexBuffer.CopyToBuffer(_physicalDeviceHandler, _deviceHandler.Get(), _graphicsQueueHandler.Get(), _graphicsCommandPoolHandler.Get(), (void*)_indices.data(), _indexBuffer.GetSize());
    _vertexBuffer.CopyToBuffer(_physicalDeviceHandler, _deviceHandler.Get(), _graphicsQueueHandler.Get(), _graphicsCommandPoolHandler.Get(), (void*)_vertices.data(), _vertexBuffer.GetSize());

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

    vkDestroyDescriptorPool(_deviceHandler.Get(), _descriptorPool, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "HelloTriangleApp Destroyed !");
#endif
}

void HelloTriangleApp::Run()
{
    MainLoop();
}
