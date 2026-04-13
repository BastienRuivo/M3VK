#include "header/Application.h"
#include "header/ApplicationInfo.h"
#include "header/CommandBuffer.h"
#include "header/GraphicsBuffer.h"
#include "header/MeshRegistry.h"
#include "header/MultiFrame.h"
#include "header/ProjectHelper.h"
#include "header/Renderer.h"
#include "header/SwapChain.h"
#include "header/DebugLayer.h"
#include <GLFW/glfw3.h>
#include <chrono>
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
    VkDescriptorSetAllocateInfo allocateInfo
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = _dynamicDescriptorPoolHandler.Get(),
        .descriptorSetCount = ApplicationInfo::Constant::MaxFrameInCount,
        .pSetLayouts = layouts.data()
    };

    MultiFrameObject<VkDescriptorSet> descriptorSet(static_cast<uint32_t>(ApplicationInfo::Constant::MaxFrameInCount));
    descriptorSet.Resize(ApplicationInfo::Constant::MaxFrameInCount);
    if(vkAllocateDescriptorSets(_deviceHandler.Get(), &allocateInfo, descriptorSet.Data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor set");
    }

    for(int i = 0; i < ApplicationInfo::Constant::MaxFrameInCount; ++i)
    {
        VkDescriptorBufferInfo cameraDataInfo
        {
            .buffer = _cameraDataBuffer.GetInternal(i),
            .offset = 0,
            .range = sizeof(CameraData)
        };

        VkDescriptorImageInfo imageInfo
        {
            .sampler = _sampler.Get(),
            .imageView = _modelImg.GetView(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

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

    CameraData cameraData =
    {
        .worldToCameraMatrix = _camera.GetViewMatrix(),
        .projectionMatrix = _camera.GetProjectionMatrix()
    };
    // it was designed for opengl so flip it
    cameraData.projectionMatrix[1][1] *= -1;

    memcpy(_cameraDataBuffer.Get(currentFrame).GetDataPtr(), &cameraData, sizeof(cameraData));
}

void Application::ResizeCallback(GLFWwindow* window, int width, int height)
{
    Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
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

    _camera._aspect = static_cast<float>(_swapChain->GetExtent().width) / static_cast<float>(_swapChain->GetExtent().height);

    _colorBackBuffer.reset();
    _colorBackBuffer = std::make_unique<GPUAllocatedImage>(_deviceHandler.Get(), _physicalDeviceHandler, _swapChain->GetExtent().width, _swapChain->GetExtent().height,
        ApplicationInfo::Get().GetMsaaSample(),
        1, _swapChain->GetImageFormat(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    _depthBuffer.reset();
    _depthBuffer = std::make_unique<GPUAllocatedImage>(_deviceHandler.Get(), _physicalDeviceHandler, _swapChain->GetExtent().width, _swapChain->GetExtent().height, ApplicationInfo::Get().GetMsaaSample(), 1, ApplicationInfo::Constant::DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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
    VkSwapchainKHR swapChain = _swapChain->Get();
    VkPresentInfoKHR presentInfo
    {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &swapChain,
        .pImageIndices = &imageIndex
    };

    result = vkQueuePresentKHR(_graphicsQueueHandler.Get(), &presentInfo);

    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
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
    VkRect2D renderArea = {0, 0, _swapChain->GetExtent().width, _swapChain->GetExtent().height};

    VkRenderingAttachmentInfo colorAttachment
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = _colorBackBuffer->GetView(),
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
        .resolveImageView = _swapChain->GetView(imageIndex),
        .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {{0.2f, 0.4f, 0.75f, 1.0f}}
    };

    VkRenderingAttachmentInfo depthAttachment
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = _depthBuffer->GetView(),
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {1.0f, 0}
    };

    // there is no stencil buffer
    VkRenderingAttachmentInfo stencilAttachment
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = VK_NULL_HANDLE,
    };

    const SwapChain::SwapChainImage& backBuffer = _swapChain->Images.Get(imageIndex);

    cmdBuffer.Begin();
    {
        _colorBackBuffer->TransitionLayoutCommand(cmdBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        _depthBuffer->TransitionLayoutCommand(cmdBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        backBuffer.TransitionLayoutCommand(cmdBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        cmdBuffer.BeginRendering(renderArea, &colorAttachment, 1, depthAttachment, stencilAttachment);
        {
            cmdBuffer.BindPipeline(_graphicsPipelineHandler.Get(), VK_PIPELINE_BIND_POINT_GRAPHICS);
            cmdBuffer.SetViewport(0, 0, renderArea.extent.width, renderArea.extent.height);
            cmdBuffer.SetScissor(renderArea);

            _meshRegistry.Bind(cmdBuffer);

            cmdBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayoutHandler.Get(), _descriptorSet.Get(currentFrame));

            for(const auto& renderer : _renderers)
            {
                renderer.Draw(cmdBuffer, _pipelineLayoutHandler.Get());
            }
        }
        cmdBuffer.EndRendering();
        backBuffer.TransitionLayoutCommand(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }
    cmdBuffer.End();
}

Application::Application() :
    // Core Window & Instance (The Foundation)
    _window(1920, 1080, "Window", this, Application::ResizeCallback, Application::MouseMoveCallback, Application::WindowFocusCallback),
    _instanceHandler(),
    _vkDebugLayer(_instanceHandler.Get()),
    _windowSurfaceHandler(_instanceHandler.Get(), _window.Get()),
    _physicalDeviceHandler(_instanceHandler.Get(), _windowSurfaceHandler.Get(), _deviceExtensions),
    _deviceHandler(_physicalDeviceHandler.Get(), _windowSurfaceHandler.Get(), _deviceExtensions),

    // Queues & Swapchain
    _graphicsQueueHandler(_deviceHandler.Get(), VkQueueHandler::Graphics),
    _presentQueueHandler(_deviceHandler.Get(), VkQueueHandler::Present),
    _swapChain(std::make_unique<SwapChain>(_window, _physicalDeviceHandler.Get(), _deviceHandler.Get(), _windowSurfaceHandler.Get())),

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
    _graphicsPipelineHandler(_swapChain->GetExtent(), _deviceHandler.Get(), ApplicationInfo::Get().GetMsaaSample(), _pipelineLayoutHandler.Get(), _swapChain->GetImageFormat(), ApplicationInfo::Constant::DepthFormat),

    // Command pool
    _graphicsCommandPoolHandler(_deviceHandler.Get()),

    // Geometry & Data Buffers
    _colorBackBuffer(std::make_unique<GPUAllocatedImage>(_deviceHandler.Get(), _physicalDeviceHandler, _swapChain->GetExtent().width, _swapChain->GetExtent().height,
        ApplicationInfo::Get().GetMsaaSample(),
        1,
        _swapChain->GetImageFormat(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)),
    _depthBuffer(std::make_unique<GPUAllocatedImage>(_deviceHandler.Get(), _physicalDeviceHandler, _swapChain->GetExtent().width, _swapChain->GetExtent().height, ApplicationInfo::Get().GetMsaaSample(), 1, ApplicationInfo::Constant::DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)),

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
    _modelImg(_deviceHandler.Get(), _physicalDeviceHandler, CPUImage("data/img/models/viking_room.png", STBI_rgb_alpha), _graphicsCommandPoolHandler.Get(), _graphicsQueueHandler.Get()),
    _sampler(_deviceHandler.Get()),
    _camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 45.0f, (float)_swapChain->GetExtent().width / (float)_swapChain->GetExtent().height, 0.1f, 100.0f),
    _meshRegistry(_deviceHandler.Get(), _physicalDeviceHandler),

    // Descriptor Sets & Execution
    _descriptorSet(CreateDescriptorSet()),
    _commandBuffer(ApplicationInfo::Constant::MaxFrameInCount, _deviceHandler.Get(), _graphicsCommandPoolHandler.Get(), _graphicsQueueHandler.Get()),

    // Synchronization
    _availableImageSemaphore(ApplicationInfo::Constant::MaxFrameInCount, _deviceHandler.Get()),
    _renderFinishedSemaphores(_swapChain->Images.Size(), _deviceHandler.Get()),
    _waitFence(ApplicationInfo::Constant::MaxFrameInCount, _deviceHandler.Get())
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "Application Creation !");
#endif

    _window.LockMouse(_mouseLocked);

    //load logo
    {
        CPUImage logo("data/img/logo.png", STBI_rgb_alpha);
        _window.SetIcon(logo.Data(), logo.Width(), logo.Height());
    }

    SubMesh vikingRoom = _meshRegistry.AddFromObj("data/models/viking_room.obj");
    _renderers.emplace_back(vikingRoom, glm::vec3(0.0f, 0.0f, 1.0f), ProjectHelper::EulerToQuat(glm::vec3(-90, -90, 0)), glm::vec3(1.0f));

    SubMesh cube = _meshRegistry.AddFromObj("data/models/Crate1.obj");

    for(uint32_t i = 0; i < 1000; ++i)
    {
        _renderers.emplace_back(cube, glm::vec3(0.0f, 0.0f,  -3.0 + -2.0f * i), ProjectHelper::EulerToQuat(glm::vec3(0, 0, 0)), glm::vec3(0.5f));
    }

    _meshRegistry.UploadAndRelease(_physicalDeviceHandler, _deviceHandler.Get(), _graphicsQueueHandler.Get(), _graphicsCommandPoolHandler.Get());
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

        for(auto& renderer : _renderers) renderer.Update(currentTime.time_since_epoch().count(), deltaTime);

        float speed = _camera.speed * deltaTime;

        if(_window.IsKeyPressed(GLFW_KEY_W)) _camera.position += speed * _camera.Front();
        if(_window.IsKeyPressed(GLFW_KEY_S)) _camera.position -= speed * _camera.Front();
        if(_window.IsKeyPressed(GLFW_KEY_A)) _camera.position -= speed * glm::normalize(glm::cross(_camera.Front(), _camera.Up()));
        if(_window.IsKeyPressed(GLFW_KEY_D)) _camera.position += speed * glm::normalize(glm::cross(_camera.Front(), _camera.Up()));
        if(_window.IsKeyPressed(GLFW_KEY_SPACE)) _camera.position += speed * _camera.Up();
        if(_window.IsKeyPressed(GLFW_KEY_LEFT_SHIFT)) _camera.position -= speed * _camera.Up();

        if(_window.IsKeyPressed(GLFW_KEY_ESCAPE)) shouldClose = true;

        if(_inputPrevent <= 0)
        {
            if(_window.IsKeyPressed(GLFW_KEY_LEFT_ALT)) _window.LockMouse(_mouseLocked = !_mouseLocked); _inputPrevent = ApplicationInfo::Constant::InputPrevent;
        }
        else
        {
            // it's in ms, deduce delta time in ms
            _inputPrevent = _inputPrevent - deltaTime * 1000;
        }

        DrawFrame();
    }

    vkDeviceWaitIdle(_deviceHandler.Get());
}

Application::~Application()
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "Application Destroyed !");
#endif

    _swapChain.reset();
}

void Application::Run()
{
    MainLoop();
}
