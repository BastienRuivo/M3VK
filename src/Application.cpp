#include "header/Application.h"
#include "header/CommandBuffer.h"
#include "header/GraphicsBuffer.h"
#include "header/MultiFrame.h"
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

#ifdef M3VK_VERBOSE_LOG
#include <string>
#endif

#ifdef M3VK_MEMORYLOG
#include <string>
#endif

#include <vector>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

MultiFrameObject<VkDescriptorSet> Application::CreateDescriptorSet()
{
    std::vector<VkDescriptorSetLayout> layouts(MaxFrameInCount, _descriptorSetLayoutHandler.Get());
    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = _descriptorPoolHandler.Get();
    allocateInfo.descriptorSetCount = static_cast<uint32_t>(MaxFrameInCount);
    allocateInfo.pSetLayouts = layouts.data();

    MultiFrameObject<VkDescriptorSet> descriptorSet(static_cast<uint32_t>(MaxFrameInCount));
    descriptorSet.Resize(MaxFrameInCount);
    if(vkAllocateDescriptorSets(_deviceHandler.Get(), &allocateInfo, descriptorSet.Data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor set");
    }

    for(int i = 0; i < MaxFrameInCount; ++i)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = _cameraDataBuffer.GetInternal(i);
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(CameraData);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet.Get(i);
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrite.pImageInfo = nullptr;
        descriptorWrite.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(_deviceHandler.Get(), 1, &descriptorWrite, 0, nullptr);
    }

    return descriptorSet;
}

void Application::UpdateCameraData(uint32_t currentFrame)
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

    memcpy(_cameraDataBuffer.Get(currentFrame).GetDataPtr(), &cameraData, sizeof(cameraData));
}

static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->FramebufferResized();
    app->UpdateWindowSize(width, height);
}

void Application::UpdateWindowSize(int width, int height)
{
    _window.ResizeWindow(width, height);
}

void Application::FramebufferResized()
{
    _framebufferResized = true;
}

void Application::RefreshSwapChain()
{
    vkDeviceWaitIdle(_deviceHandler.Get());

    // TODO : Swap chain is currently resetted the first frame beacause it is out of date
    _swapChain.reset();
    _swapChain = std::make_unique<SwapChain>(_window, _physicalDeviceHandler.Get(), _deviceHandler.Get(), _windowSurfaceHandler.Get());

    _framebuffer.Clear();
    InitFramebuffer(_framebuffer);
}

void Application::DrawFrame()
{
    _waitFence.Get(_currentFrame).Wait(UINT64_MAX);

    // Acquire image to draw on
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_deviceHandler.Get(), _swapChain->_internal, UINT64_MAX, _availableImageSemaphore.GetInternal(_currentFrame), VK_NULL_HANDLE, &imageIndex);

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
    _waitFence.Get(_currentFrame).Reset();

    UpdateCameraData(_currentFrame);

    _commandBuffer.Get(_currentFrame).Reset();
    RecordCommandBuffer(_commandBuffer.Get(_currentFrame), _currentFrame, imageIndex);

    // stackallocs that can be cached.
    VkSemaphore wait[] = {_availableImageSemaphore.GetInternal(_currentFrame)};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphore[] = {_renderFinishedSemaphores.GetInternal(imageIndex)};

    _commandBuffer.Get(_currentFrame).Submit(wait, 1, waitStages, signalSemaphore, 1, _waitFence.GetInternal(_currentFrame));

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

    _currentFrame = (_currentFrame + 1) % Application::MaxFrameInCount;
}

void Application::RecordCommandBuffer(CommandBuffer cmdBuffer, uint32_t currentFrame, uint32_t imageIndex)
{
    cmdBuffer.Begin();
    {
        cmdBuffer.BeginRenderPass(_renderPassHandler.Get(), _framebuffer.GetInternal(imageIndex), {0, 0, 0, 0}, _swapChain->Extent);
        {
            cmdBuffer.BindPipeline(_graphicsPipelineHandler.Get(), VK_PIPELINE_BIND_POINT_GRAPHICS);
            cmdBuffer.SetViewport(_swapChain->Extent.width, _swapChain->Extent.height);
            cmdBuffer.SetScissor(0, 0, _swapChain->Extent.width, _swapChain->Extent.height);
            cmdBuffer.BindBuffer(_vertexBuffer);
            cmdBuffer.BindBuffer(_indexBuffer);
            cmdBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayoutHandler.Get(), _descriptorSet.Get(currentFrame));
            cmdBuffer.DrawIndexed(static_cast<uint32_t>(_indices.size()));
        }
        cmdBuffer.EndRenderPass();
    }
    cmdBuffer.End();
}

MultiFrameHandler<VkFramebufferHandler> Application::CreateFramebuffer()
{
    MultiFrameHandler<VkFramebufferHandler> framebuffer(_swapChain->ImageViews.size());

    InitFramebuffer(framebuffer);

    return framebuffer;
}

void Application::InitFramebuffer(MultiFrameHandler<VkFramebufferHandler>& framebuffer)
{
    for(size_t i = 0; i < _swapChain->ImageViews.size(); ++i)
    {
        std::vector<VkImageView> attachments = {
            _swapChain->ImageViews[i]
        };
        // emplace back to avoid a temp obejct
        framebuffer.EmplaceBack(_deviceHandler.Get(), _renderPassHandler.Get(), _swapChain->Extent, attachments);
    }
}

Application::Application() :
    _waitFence(MaxFrameInCount, _deviceHandler.Get()),
    _availableImageSemaphore(MaxFrameInCount, _deviceHandler.Get()),
    _renderFinishedSemaphores(_swapChain->Images.size(), _deviceHandler.Get()),
    _commandBuffer(static_cast<uint32_t>(MaxFrameInCount), _deviceHandler.Get(), _graphicsCommandPoolHandler.Get(), _graphicsQueueHandler.Get()),
    _descriptorSet(CreateDescriptorSet()),
    _descriptorPoolHandler(_deviceHandler.Get(), static_cast<uint32_t>(MaxFrameInCount)),
    _cameraDataBuffer(MaxFrameInCount, _physicalDeviceHandler, _deviceHandler.Get(), 1, sizeof(CameraData), GraphicsBuffer::UNIFORM),
    _vertexBuffer(_physicalDeviceHandler, _deviceHandler.Get(), _vertices.size(), sizeof(_vertices[0]), GraphicsBuffer::BufferType::VERTEX),
    _indexBuffer(_physicalDeviceHandler, _deviceHandler.Get(), _indices.size(), sizeof(_indices[0]), GraphicsBuffer::BufferType::INDEX),
    _graphicsCommandPoolHandler(_deviceHandler.Get(), _physicalDeviceHandler.QueueFamilyIds),
    _framebuffer(CreateFramebuffer()),
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
    DebugLayer::Log(DebugLayer::LogType::CREATE, "Application Creation !");
#endif

    // init data
    _indexBuffer.CopyToBuffer(_physicalDeviceHandler, _deviceHandler.Get(), _graphicsQueueHandler.Get(), _graphicsCommandPoolHandler.Get(), (void*)_indices.data(), _indexBuffer.GetSize());
    _vertexBuffer.CopyToBuffer(_physicalDeviceHandler, _deviceHandler.Get(), _graphicsQueueHandler.Get(), _graphicsCommandPoolHandler.Get(), (void*)_vertices.data(), _vertexBuffer.GetSize());
}

void Application::MainLoop()
{
    while (!_window.ShouldClose())
    {
        _window.ProcessEvent();
        DrawFrame();
    }

    vkDeviceWaitIdle(_deviceHandler.Get());
}

Application::~Application()
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "Application Destroyed !");
#endif
}

void Application::Run()
{
    MainLoop();
}
