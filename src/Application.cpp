#include "header/Application.h"
#include "header/ApplicationInfo.h"
#include "header/CommandBuffer.h"
#include "header/GraphicsBuffer.h"
#include "header/Image.h"
#include "header/Mesh.h"
#include "header/MultiFrame.h"
#include "header/SwapChain.h"
#include "header/DebugLayer.h"
#include "header/Vertex.h"
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
#include <initializer_list>
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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

MultiFrameObject<VkDescriptorSet> Application::CreateDescriptorSet()
{
    std::vector<VkDescriptorSetLayout> layouts(ApplicationInfo::Constant::MaxFrameInCount, _descriptorSetLayoutHandler.Get());
    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = _dynamicDescriptorPoolHandler.Get();
    allocateInfo.descriptorSetCount = ApplicationInfo::Constant::MaxFrameInCount;
    allocateInfo.pSetLayouts = layouts.data();

    MultiFrameObject<VkDescriptorSet> descriptorSet(static_cast<uint32_t>(ApplicationInfo::Constant::MaxFrameInCount));
    descriptorSet.Resize(ApplicationInfo::Constant::MaxFrameInCount);
    if(vkAllocateDescriptorSets(_deviceHandler.Get(), &allocateInfo, descriptorSet.Data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor set");
    }

    for(int i = 0; i < ApplicationInfo::Constant::MaxFrameInCount; ++i)
    {
        VkDescriptorBufferInfo cameraDataInfo{};
        cameraDataInfo.buffer = _cameraDataBuffer.GetInternal(i);
        cameraDataInfo.offset = 0;
        cameraDataInfo.range = sizeof(CameraData);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = _modelImg.GetView();
        imageInfo.sampler = _sampler.Get();

        uint32_t binding = 0;
        std::vector<VkWriteDescriptorSet> descriptorWrites
        {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptorSet.Get(i),
                .dstBinding = binding++,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &cameraDataInfo,
                .pTexelBufferView = nullptr
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptorSet.Get(i),
                .dstBinding = binding++,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &imageInfo,
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr
            }
        };

        vkUpdateDescriptorSets(_deviceHandler.Get(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    return descriptorSet;
}

void Application::UpdateCameraData(uint32_t currentFrame)
{
    VkExtent2D extent = _swapChain->GetExtent();

    CameraData cameraData = {};
    cameraData.worldToCameraMatrix = _camera.GetViewMatrix();
    cameraData.projectionMatrix = _camera.GetProjectionMatrix();
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

void Application::WindowFocusCallback(GLFWwindow* window, int focused)
{
    Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

    if(focused)
    {
        app->_inputDeltaPrevent = 3;
    }
}

void Application::MouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
{
    Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

    if(app->_inputDeltaPrevent > 0)
    {
        app->_lastMouseX = xpos;
        app->_lastMouseY = ypos;
        app->_inputDeltaPrevent--;
        return;
    }

    double dx = xpos - app->_lastMouseX;
    double dy = ypos - app->_lastMouseY;

    const float sensitivity = 0.1f;

    app->_camera.Rotate(dx * sensitivity, dy * sensitivity);

    app->_lastMouseX = xpos;
    app->_lastMouseY = ypos;
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
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_window.Get(), &width, &height);
        glfwPollEvents();
    }

    vkDeviceWaitIdle(_deviceHandler.Get());

    // TODO : Swap chain is currently resetted the first frame beacause it is out of date
    _swapChain.reset();
    _swapChain = std::make_unique<SwapChain>(_window, _physicalDeviceHandler.Get(), _deviceHandler.Get(), _windowSurfaceHandler.Get());

    _colorBackBuffer.reset();
    _colorBackBuffer = std::make_unique<GPUImage>(_deviceHandler.Get(), _physicalDeviceHandler, _swapChain->GetExtent().width, _swapChain->GetExtent().height,
        ApplicationInfo::Get().GetMsaaSample(),
        1, _swapChain->GetImageFormat(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    _depthBuffer.reset();
    _depthBuffer = std::make_unique<GPUImage>(_deviceHandler.Get(), _physicalDeviceHandler, _swapChain->GetExtent().width, _swapChain->GetExtent().height, ApplicationInfo::Get().GetMsaaSample(), 1, DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),

    _framebuffer.Clear();
    InitFramebuffer(_framebuffer);
}

void Application::DrawFrame()
{
    _waitFence.Get(_currentFrame).Wait(UINT64_MAX);

    // Acquire image to draw on
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_deviceHandler.Get(), _swapChain->Get(), UINT64_MAX, _availableImageSemaphore.GetInternal(_currentFrame), VK_NULL_HANDLE, &imageIndex);

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

    VkSwapchainKHR swapChains[] = {_swapChain->Get()};
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

    _currentFrame = (_currentFrame + 1) % ApplicationInfo::Constant::MaxFrameInCount;
}

void Application::RecordCommandBuffer(const CommandBuffer& cmdBuffer, uint32_t currentFrame, uint32_t imageIndex)
{
    VkExtent2D renderExtent = _swapChain->GetExtent();

    ObjectData objectData = {};
    objectData.localToWorldMatrix = glm::mat4(1.0f);
    objectData.localToWorldMatrix = glm::rotate<float>(objectData.localToWorldMatrix, glm::radians(-90.0f), glm::vec3(1, 0, 0));
    objectData.localToWorldMatrix = glm::rotate<float>(objectData.localToWorldMatrix, glm::radians(-90.0f), glm::vec3(0, 0, 1));

    cmdBuffer.Begin();
    {
        cmdBuffer.BeginRenderPass(_renderPassHandler.Get(), _framebuffer.GetInternal(imageIndex), clearValues, renderExtent);
        {
            cmdBuffer.BindPipeline(_graphicsPipelineHandler.Get(), VK_PIPELINE_BIND_POINT_GRAPHICS);
            cmdBuffer.SetViewport(renderExtent.width, renderExtent.height);
            cmdBuffer.SetScissor(0, 0, renderExtent.width, renderExtent.height);

            cmdBuffer.PushConstants(_pipelineLayoutHandler.Get(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ObjectData), &objectData);

            cmdBuffer.BindBuffer(*_vertexBuffer);
            cmdBuffer.BindBuffer(*_indexBuffer);
            cmdBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayoutHandler.Get(), _descriptorSet.Get(currentFrame));
            cmdBuffer.DrawIndexed(static_cast<uint32_t>(_indexBuffer->GetCount()));
        }
        cmdBuffer.EndRenderPass();
    }
    cmdBuffer.End();
}

MultiFrameHandler<VkFramebufferHandler> Application::CreateFramebuffer()
{
    MultiFrameHandler<VkFramebufferHandler> framebuffer(_swapChain->ImageViews.Size());

    InitFramebuffer(framebuffer);

    return framebuffer;
}

void Application::InitFramebuffer(MultiFrameHandler<VkFramebufferHandler>& framebuffer)
{
    for(size_t i = 0; i < _swapChain->ImageViews.Size(); ++i)
    {
        std::vector<VkImageView> attachments = {
            _colorBackBuffer->GetView(),
            _depthBuffer->GetView(),
            _swapChain->ImageViews.GetInternal(i)
        };
        // emplace back to avoid a temp obejct
        framebuffer.EmplaceBack(_deviceHandler.Get(), _renderPassHandler.Get(), _swapChain->GetExtent(), attachments);
    }
}

Application::Application() :
    // Core Window & Instance (The Foundation)
    _window(1920, 1080, "Window", this, FramebufferResizeCallback, Application::MouseMoveCallback, Application::WindowFocusCallback),
    _instanceHandler(),
    _vkDebugLayer(_instanceHandler.Get()),
    _windowSurfaceHandler(_instanceHandler.Get(), _window.Get()),
    _physicalDeviceHandler(_instanceHandler.Get(), _windowSurfaceHandler.Get(), _deviceExtensions),
    _deviceHandler(_physicalDeviceHandler.Get(), _windowSurfaceHandler.Get(), _deviceExtensions),

    // Queues & Swapchain
    _graphicsQueueHandler(_deviceHandler.Get(), VkQueueHandler::Graphics),
    _presentQueueHandler(_deviceHandler.Get(), VkQueueHandler::Present),
    _swapChain(std::make_unique<SwapChain>(_window, _physicalDeviceHandler.Get(), _deviceHandler.Get(), _windowSurfaceHandler.Get())),

    // Render Layouts & Pipelines
    _renderPassHandler(_deviceHandler.Get(), ApplicationInfo::Get().GetMsaaSample(), _swapChain->GetImageFormat(), DepthFormat),

    _descriptorSetLayoutHandler(_deviceHandler.Get(), std::initializer_list<VkDescriptorSetLayoutBinding>(
        {
            VkDescriptorSetLayoutBinding{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr },
            VkDescriptorSetLayoutBinding{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr }
        }
    )),
    _pipelineLayoutHandler(_deviceHandler.Get(), std::initializer_list<VkDescriptorSetLayout>(
        {
            _descriptorSetLayoutHandler.Get()
        }
    ),
    std::initializer_list<VkPushConstantRange>(
        {
            VkPushConstantRange{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ObjectData) }
        }
    )),
    _graphicsPipelineHandler(_swapChain->GetExtent(), _deviceHandler.Get(), ApplicationInfo::Get().GetMsaaSample(), _pipelineLayoutHandler.Get(), _renderPassHandler.Get()),

    // Framebuffers & Command Pools
    _framebuffer(CreateFramebuffer()),
    _graphicsCommandPoolHandler(_deviceHandler.Get()),

    // Geometry & Data Buffers
    _colorBackBuffer(std::make_unique<GPUImage>(_deviceHandler.Get(), _physicalDeviceHandler, _swapChain->GetExtent().width, _swapChain->GetExtent().height,
        ApplicationInfo::Get().GetMsaaSample(),
        1,
        _swapChain->GetImageFormat(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)),
    _depthBuffer(std::make_unique<GPUImage>(_deviceHandler.Get(), _physicalDeviceHandler, _swapChain->GetExtent().width, _swapChain->GetExtent().height, ApplicationInfo::Get().GetMsaaSample(), 1, DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)),

    // Descriptor Pools
    _dynamicDescriptorPoolHandler(_deviceHandler.Get(), std::initializer_list<VkDescriptorPoolSize>(
        {
            VkDescriptorPoolSize
            {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = ApplicationInfo::Constant::MaxFrameInCount
            },
            VkDescriptorPoolSize
            {
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = ApplicationInfo::Constant::MaxFrameInCount
            },
            VkDescriptorPoolSize
            {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = ApplicationInfo::Constant::MaxFrameInCount
            }
        }), ApplicationInfo::Constant::MaxFrameInCount),
    _staticDescriptorPoolHandler(_deviceHandler.Get(), std::initializer_list<VkDescriptorPoolSize>(
        {
            VkDescriptorPoolSize
            {
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1
            },
            VkDescriptorPoolSize
            {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1
            }
        }), 1),

    _cameraDataBuffer(ApplicationInfo::Constant::MaxFrameInCount, _physicalDeviceHandler, _deviceHandler.Get(), 1, sizeof(CameraData), GraphicsBuffer::UNIFORM),
    _modelImg(_deviceHandler.Get(), _physicalDeviceHandler, CPUImage("data/img/kirbo.png", STBI_rgb_alpha), _graphicsCommandPoolHandler.Get(), _graphicsQueueHandler.Get()),
    _sampler(_deviceHandler.Get()),
    _camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 45.0f, (float)_swapChain->GetExtent().width / (float)_swapChain->GetExtent().height, 0.1f, 100.0f),
    _vertexBuffer(std::make_unique<MemoryBuffer>(_physicalDeviceHandler, _deviceHandler.Get(), ApplicationInfo::Constant::VertexBufferMaxSize, sizeof(Vertex), GraphicsBuffer::BufferType::VERTEX)),
    _indexBuffer(std::make_unique<MemoryBuffer>(_physicalDeviceHandler, _deviceHandler.Get(), ApplicationInfo::Constant::IndexBufferMaxSize, sizeof(uint32_t), GraphicsBuffer::BufferType::INDEX)),

    // Descriptor Sets & Execution
    _descriptorSet(CreateDescriptorSet()),
    _commandBuffer(ApplicationInfo::Constant::MaxFrameInCount, _deviceHandler.Get(), _graphicsCommandPoolHandler.Get(), _graphicsQueueHandler.Get()),

    // Synchronization
    _availableImageSemaphore(ApplicationInfo::Constant::MaxFrameInCount, _deviceHandler.Get()),
    _renderFinishedSemaphores(_swapChain->Images.size(), _deviceHandler.Get()),
    _waitFence(ApplicationInfo::Constant::MaxFrameInCount, _deviceHandler.Get())
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "Application Creation !");
#endif

    //load logo
    {
        CPUImage logo("data/img/logo.png", STBI_rgb_alpha);
        _window.SetIcon(logo.Data(), logo.Width(), logo.Height());
    }

    VkClearValue colorClear;
    colorClear.color = {0.2f, 0.4f, 0.75f, 1.0f};
    VkClearValue depthClear;
    depthClear.depthStencil = {1.0f, 0};

    clearValues.reserve(2);
    clearValues.push_back(colorClear);
    clearValues.push_back(depthClear);

    Mesh mesh;
    mesh.LoadFromObj("data/models/Crate1.obj");
    mesh.UploadAndRelease(_physicalDeviceHandler, _deviceHandler.Get(), _graphicsQueueHandler.Get(), _graphicsCommandPoolHandler.Get(),
        *_vertexBuffer,
        *_indexBuffer);
}

void Application::MainLoop()
{
    bool shouldClose = false;
    while (!_window.ShouldClose() && !shouldClose)
    {
        _window.ProcessEvent();

        auto currentTime = std::chrono::high_resolution_clock::now();

        auto deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - _lastFrameTime).count();
        _lastFrameTime = currentTime;

        float speed = _camera.speed * deltaTime;

        if(_window.IsKeyPressed(GLFW_KEY_W)) _camera.position += speed * _camera.Front();
        if(_window.IsKeyPressed(GLFW_KEY_S)) _camera.position -= speed * _camera.Front();
        if(_window.IsKeyPressed(GLFW_KEY_A)) _camera.position -= speed * glm::normalize(glm::cross(_camera.Front(), _camera.Up()));
        if(_window.IsKeyPressed(GLFW_KEY_D)) _camera.position += speed * glm::normalize(glm::cross(_camera.Front(), _camera.Up()));
        if(_window.IsKeyPressed(GLFW_KEY_SPACE)) _camera.position += speed * _camera.Up();
        if(_window.IsKeyPressed(GLFW_KEY_LEFT_SHIFT)) _camera.position -= speed * _camera.Up();

        if(_window.IsKeyPressed(GLFW_KEY_ESCAPE)) shouldClose = true;

        DrawFrame();
    }

    vkDeviceWaitIdle(_deviceHandler.Get());
}

Application::~Application()
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "Application Destroyed !");
#endif

    _vertexBuffer.reset();
    _indexBuffer.reset();
}

void Application::Run()
{
    MainLoop();
}
