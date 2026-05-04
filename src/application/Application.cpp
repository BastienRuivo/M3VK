#include "application/Application.h"
#include "application/ApplicationHelper.h"
#include "rendering/GraphicsBuffer.h"
#include "application/ApplicationInfo.h"
#include "rendering/CommandBuffer.h"
#include "rendering/DescriptorPool.h"
#include "rendering/GPUImage.h"
#include "rendering/GraphicsBuffer.h"
#include "registry/MaterialRegistry.h"
#include "registry/MeshRegistry.h"
#include "rendering/ImageHelper.h"
#include "rendering/MultiFrame.h"
#include "asset/AssetHelper.h"
#include "asset/MeshHelper.h"
#include "registry/Registry.h"
#include "rendering/SwapChain.h"
#include "application/DebugLayer.h"
#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/trigonometric.hpp>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include "asset/CPUImage.h"

#ifdef M3VK_VERBOSE_LOG
#include <string>
#endif

#ifdef M3VK_MEMORYLOG
#include <string>
#endif

#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

MultiFrameObject<DescriptorSetHandle> Application::CreateDescriptorSet()
{
    MultiFrameObject<DescriptorSetHandle> descriptorSets(_dynamicDescriptorPool.Allocate(0, ApplicationInfo::Constant::MaxFrameInCount));

    for(int i = 0; i < ApplicationInfo::Constant::MaxFrameInCount; ++i)
    {
        VkDescriptorBufferInfo cameraDataInfo = DescriptorPool::DescriptorBufferInfo(_cameraDataBuffer.Get(i), 0);

        uint32_t binding = 0;
        DescriptorHelper::UpdateDescriptorSet(std::initializer_list<VkWriteDescriptorSet>(
            {
                {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptorSets.Get(i).set,
                .dstBinding = binding++,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &cameraDataInfo,
                .pTexelBufferView = nullptr
                }
            }
        ), {});
    }

    return descriptorSets;
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

    cameraData.viewProjectionMatrix = cameraData.projectionMatrix * cameraData.worldToCameraMatrix;

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
        glfwGetFramebufferSize(_window.Internal(), &width, &height);
        glfwPollEvents();
    }

    vkDeviceWaitIdle(ApplicationInfo::Device());

    // TODO : Swap chain is currently resetted the first frame beacause it is out of date
    _swapChain.reset();
    _swapChain = std::make_unique<SwapChain>(_window, _windowSurfaceHandler.Internal());

    _camera._aspect = static_cast<float>(_swapChain->GetExtent().width) / static_cast<float>(_swapChain->GetExtent().height);

    _colorBackBuffer.reset();
    _colorBackBuffer = std::make_unique<GPUAllocatedImage>(_swapChain->GetExtent().width, _swapChain->GetExtent().height,
        ApplicationInfo::Get().GetMsaaSample(),
        1, _swapChain->GetImageFormat(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    _depthBuffer.reset();
    _depthBuffer = std::make_unique<GPUAllocatedImage>(_swapChain->GetExtent().width, _swapChain->GetExtent().height, ApplicationInfo::Get().GetMsaaSample(), 1, ApplicationInfo::Constant::DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void Application::DrawFrame()
{
    _waitFence.Get(_currentFrame).Wait(UINT64_MAX);

    // Acquire image to draw on
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(ApplicationInfo::Device(), _swapChain->Internal(), UINT64_MAX, _availableImageSemaphore.Internal(_currentFrame), VK_NULL_HANDLE, &imageIndex);

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
    VkSemaphore wait[] = {_availableImageSemaphore.Internal(_currentFrame)};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphore[] = {_renderFinishedSemaphores.Internal(imageIndex)};

    _commandBuffer.Get(_currentFrame).Submit(wait, 1, waitStages, signalSemaphore, 1, _waitFence.Internal(_currentFrame));

    // actually present the frame
    VkSwapchainKHR swapChain = _swapChain->Internal();
    VkPresentInfoKHR presentInfo
    {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &swapChain,
        .pImageIndices = &imageIndex
    };

    result = vkQueuePresentKHR(_graphicsQueueHandler.Internal(), &presentInfo);

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
        .imageView = _colorBackBuffer->View(),
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
        .resolveImageView = _swapChain->View(imageIndex),
        .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {{0.2f, 0.4f, 0.75f, 1.0f}}
    };

    VkRenderingAttachmentInfo depthAttachment
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = _depthBuffer->View(),
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

    const ImageReference& backBuffer = _swapChain->Images.Get(imageIndex);

    cmdBuffer.Begin();
    {
        ImageHelper::TransitionLayoutCommand(cmdBuffer, _colorBackBuffer->Internal(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        ImageHelper::TransitionLayoutCommand(cmdBuffer, _depthBuffer->Internal(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        ImageHelper::TransitionLayoutCommand(cmdBuffer, backBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        cmdBuffer.BeginRendering(renderArea, &colorAttachment, 1, depthAttachment, stencilAttachment);
        {
            cmdBuffer.BindPipeline(_graphicsPipelineHandler.Internal(), VK_PIPELINE_BIND_POINT_GRAPHICS);
            cmdBuffer.SetViewport(0, 0, renderArea.extent.width, renderArea.extent.height);
            cmdBuffer.SetScissor(renderArea);

            for(const auto& registry : _registries)
            {
                registry->Bind(cmdBuffer, _pipelineLayoutHandler.Internal());
            }

            cmdBuffer.BindDescriptorSets(_pipelineLayoutHandler.Internal(), _descriptorSet.Get(currentFrame), 0);
            cmdBuffer.BindDescriptorSets(_pipelineLayoutHandler.Internal(), _materialInstancesSet, 2);

            auto& meshRegistry = static_cast<MeshRegistry&>(*_registries[(size_t)RegistryType::Mesh]);

            meshRegistry.Draw(cmdBuffer, _pipelineLayoutHandler.Internal());

        }
        cmdBuffer.EndRendering();
        ImageHelper::TransitionLayoutCommand(cmdBuffer, backBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }
    cmdBuffer.End();
}

uint32_t Application::LoadDefaultMaterial()
{
    MaterialRegistry& materialRegistry = static_cast<MaterialRegistry&>(*_registries[(size_t)RegistryType::Material]);

    GPUAllocatedImage baseColorTex = GPUAllocatedImage(AssetHelper::ImageFromCPU(CPUImage("data/default/BaseColor.png", STBI_rgb_alpha), _graphicsCommandPoolHandler.Internal(), _graphicsQueueHandler.Internal()));
    uint32_t baseIndex = materialRegistry.RegisterTexture(std::move(baseColorTex), _sampler.Internal());

    GPUAllocatedImage normalMapTex = GPUAllocatedImage(AssetHelper::ImageFromCPU(CPUImage("data/default/BaseNormal.png", STBI_rgb_alpha), _graphicsCommandPoolHandler.Internal(), _graphicsQueueHandler.Internal()));
    uint32_t normalIndex = materialRegistry.RegisterTexture(std::move(normalMapTex), _sampler.Internal());

    GPUAllocatedImage mraoTex = GPUAllocatedImage(AssetHelper::ImageFromCPU(CPUImage("data/default/BaseMRAO.png", STBI_rgb_alpha), _graphicsCommandPoolHandler.Internal(), _graphicsQueueHandler.Internal()));
    uint32_t mraoIndex = materialRegistry.RegisterTexture(std::move(mraoTex), _sampler.Internal());

    MaterialProperties matProperties = MaterialProperties::Default();
    matProperties.BaseColorTexId = baseIndex;
    matProperties.NormalMapTexId = normalIndex;
    matProperties.MRAOTexId = mraoIndex;

    return materialRegistry.RegisterMaterial(matProperties);
}

Application::Application() :
    // Core Window & Instance (The Foundation)
    _window(1920, 1080, "Window", this, Application::ResizeCallback, Application::MouseMoveCallback, Application::WindowFocusCallback),
    _instanceHandler(),
    _vkDebugLayer(_instanceHandler.Internal()),
    _windowSurfaceHandler(_instanceHandler.Internal(), _window.Internal()),
    _physicalDeviceHandler(_instanceHandler.Internal(), _windowSurfaceHandler.Internal(), _deviceExtensions),
    _deviceHandler(_windowSurfaceHandler.Internal(), _deviceExtensions),

    // Queues & Swapchain
    _graphicsQueueHandler(VkQueueHandler::Graphics),
    _presentQueueHandler(VkQueueHandler::Present),
    _swapChain(std::make_unique<SwapChain>(_window, _windowSurfaceHandler.Internal())),

    _dynamicDescriptorPool(DescriptorPool::Builder()
        .AddLayout(
            DescriptorPool::LayoutBuilder()
                .AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1)
            )
        .SetFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
        .SetMaxSets(ApplicationInfo::Constant::MaxFrameInCount)
        .Build()),
    _staticDescriptorPool(DescriptorPool::Builder()
        .SetMaxSets(2)
        .SetFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT)
        .AddLayout(DescriptorPool::LayoutBuilder()
            .AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
                | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
                | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT, ApplicationInfo::Constant::MaxTextureCount)
            .SetFlags(VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT)
        )
        .AddLayout(DescriptorPool::LayoutBuilder()
            .AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1) // Material
            .AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1) // InstanceData
        )
        .Build()
    ),
    _materialInstancesSet(_staticDescriptorPool.Allocate(1)),
    _registries(
        {
            std::make_unique<MeshRegistry>(),
            std::make_unique<MaterialRegistry>(_staticDescriptorPool, 0),
        }),
    _pipelineLayoutHandler(std::initializer_list<VkDescriptorSetLayout>(
        {
            _dynamicDescriptorPool.Layout(0),
            _staticDescriptorPool.Layout(0),
            _staticDescriptorPool.Layout(1)
        }
    ),
    std::initializer_list<VkPushConstantRange>(
        // {
        //     VkPushConstantRange{ VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ObjectData) }
        // }
    )),
    _graphicsPipelineHandler(_swapChain->GetExtent(), ApplicationInfo::Get().GetMsaaSample(), _pipelineLayoutHandler.Internal(), _swapChain->GetImageFormat(), ApplicationInfo::Constant::DepthFormat),

    // Command pool
    _graphicsCommandPoolHandler(ApplicationInfo::GetGraphicsQueueId()),

    // Geometry & Data Buffers
    _colorBackBuffer(std::make_unique<GPUAllocatedImage>(_swapChain->GetExtent().width, _swapChain->GetExtent().height,
        ApplicationInfo::Get().GetMsaaSample(),
        1,
        _swapChain->GetImageFormat(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)),
    _depthBuffer(std::make_unique<GPUAllocatedImage>(_swapChain->GetExtent().width, _swapChain->GetExtent().height, ApplicationInfo::Get().GetMsaaSample(), 1, ApplicationInfo::Constant::DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)),

    _cameraDataBuffer(ApplicationInfo::Constant::MaxFrameInCount, 1, sizeof(CameraData), GraphicsBuffer::DYNAMIC_UNIFORM),
    _sampler(),
    _camera(glm::vec3(0.0f, 0.5f, 4.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 45.0f, (float)_swapChain->GetExtent().width / (float)_swapChain->GetExtent().height, 0.1f, 1000.0f),
    // Descriptor Sets & Execution
    _descriptorSet(CreateDescriptorSet()),
    _commandBuffer(ApplicationInfo::Constant::MaxFrameInCount, _graphicsCommandPoolHandler.Internal(), _graphicsQueueHandler.Internal()),

    // Synchronization
    _availableImageSemaphore(ApplicationInfo::Constant::MaxFrameInCount),
    _renderFinishedSemaphores(_swapChain->Images.Size()),
    _waitFence(ApplicationInfo::Constant::MaxFrameInCount)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "Application Creation !");
#endif
    _window.LockMouse(_mouseLocked);
    {
        CPUImage logo("data/logo.png", STBI_rgb_alpha);
        _window.SetIcon(logo.Data(), logo.Width(), logo.Height());
    }

    MeshRegistry& meshRegistry = static_cast<MeshRegistry&>(*_registries[(size_t)RegistryType::Mesh]);
    MaterialRegistry& materialRegistry = static_cast<MaterialRegistry&>(*_registries[(size_t)RegistryType::Material]);

    VkDescriptorBufferInfo materialBufferInfo = materialRegistry.MaterialBufferInfo();
    VkDescriptorBufferInfo instanceBufferInfo = meshRegistry.InstanceBufferInfo();
    DescriptorHelper::UpdateDescriptorSet(std::initializer_list<VkWriteDescriptorSet>(
        {
            VkWriteDescriptorSet
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = _materialInstancesSet.set,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &materialBufferInfo
            },
            VkWriteDescriptorSet
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = _materialInstancesSet.set,
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &instanceBufferInfo
            }
        }
    ), {});

    uint32_t defaultMaterial = LoadDefaultMaterial();

    const float axisLength = 10.0f;
    const float axisThickness = 0.00625f;

    uint32_t cube = MeshHelper::CubeMesh(meshRegistry);
    MaterialProperties defaultMat = materialRegistry.Material(defaultMaterial);

    {
        defaultMat.BaseColor = {1.0f, 0.0f, 0.0f, 1.0f};
        uint32_t matBinding = materialRegistry.RegisterMaterial(defaultMat);
        InstanceData instance = {
            .modelMatrix = ApplicationHelper::TranslateRotateScale(glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(axisLength, axisThickness, axisThickness)),
            .materialId = matBinding,
        };
        meshRegistry.RegisterInstance(instance);
    }

    {
        defaultMat.BaseColor = {0.0f, 1.0f, 0.0f, 1.0f};
        uint32_t matBinding = materialRegistry.RegisterMaterial(defaultMat);
        InstanceData instance = {
            .modelMatrix = ApplicationHelper::TranslateRotateScale(glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(axisThickness, axisLength, axisThickness)),
            .materialId = matBinding,
        };
        meshRegistry.RegisterInstance(instance);
    }

    {
        defaultMat.BaseColor = {0.0f, 0.0f, 1.0f, 1.0f};
        uint32_t matBinding = materialRegistry.RegisterMaterial(defaultMat);
        InstanceData instance = {
            .modelMatrix = ApplicationHelper::TranslateRotateScale(glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(axisThickness, axisThickness, axisLength)),
            .materialId = matBinding,
        };
        meshRegistry.RegisterInstance(instance);
    }

    AssetHelper::Load3DModel("data/MinecraftChest.m3vkasset", meshRegistry, materialRegistry, _graphicsCommandPoolHandler.Internal(), _graphicsQueueHandler.Internal(), _sampler.Internal());

    InstanceData instance = {
        .modelMatrix = ApplicationHelper::TranslateRotateScale(glm::vec3(3.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 90.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f)),
        .materialId = materialRegistry.MaterialsCount() - 1,
    };
    meshRegistry.RegisterInstance(instance);

    // for(uint32_t i = 0; i < 100; ++i)
    // {
    //     float red = (rand() / (float)RAND_MAX);
    //     float green = (rand() / (float)RAND_MAX);
    //     float blue = (rand() / (float)RAND_MAX);
    //     defaultMat.BaseColor = {red, green, blue, 1.0f};
    //     uint32_t matBinding = materialRegistry.RegisterMaterial(defaultMat);
    // }

    // for(uint32_t i = 0; i < 100000; ++i)
    // {
    //     uint32_t matBinding = rand() % materialRegistry.MaterialsCount();

    //     // rand float 100
    //     float x = (rand() / (float)RAND_MAX) * 80;
    //     float y = (rand() / (float)RAND_MAX) * 80;
    //     float z = (rand() / (float)RAND_MAX) * 80;

    //     float scale = (rand() / (float)RAND_MAX) * 25;
    //     InstanceData instance = {
    //         .modelMatrix = ApplicationHelper::TranslateRotateScale(glm::vec3(x, y, z),
    //             glm::vec3(0.0f, 0.0f, 0.0f),
    //             glm::vec3(axisThickness * scale, axisThickness * scale, axisThickness * scale)),
    //         .materialId = matBinding,
    //     };
    //     meshRegistry.RegisterInstance(instance);
    // }

    for(auto& registry : _registries)
    {
        registry->UploadAndRelease(_graphicsQueueHandler.Internal(), _graphicsCommandPoolHandler.Internal());
    }
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

        if(_window.IsKeyPressed(GLFW_KEY_LEFT_SHIFT)) speed *= 10.0;

        if(_window.IsKeyPressed(GLFW_KEY_W)) _camera.position += speed * _camera.Front();
        if(_window.IsKeyPressed(GLFW_KEY_S)) _camera.position -= speed * _camera.Front();
        if(_window.IsKeyPressed(GLFW_KEY_A)) _camera.position -= speed * glm::normalize(glm::cross(_camera.Front(), _camera.Up()));
        if(_window.IsKeyPressed(GLFW_KEY_D)) _camera.position += speed * glm::normalize(glm::cross(_camera.Front(), _camera.Up()));
        if(_window.IsKeyPressed(GLFW_KEY_E)) _camera.position += speed * _camera.Up();
        if(_window.IsKeyPressed(GLFW_KEY_Q)) _camera.position -= speed * _camera.Up();

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

    vkDeviceWaitIdle(ApplicationInfo::Device());
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
