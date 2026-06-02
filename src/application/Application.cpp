#include "application/Application.h"
#include "application/ApplicationHelper.h"
#include "rendering/DescriptorAllocator.h"
#include "rendering/GraphicsBuffer.h"
#include "application/ApplicationInfo.h"
#include "rendering/CommandBuffer.h"
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
#include <memory>
#include <stdexcept>
#include "asset/CPUImage.h"

#include "ShaderBindings.h"

#ifdef M3VK_VERBOSE_LOG
#include <string>
#endif

#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>



void Application::UpdateCameraData()
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

    memcpy(_cameraDataBuffer.GetDataPtr(), &cameraData, sizeof(cameraData));
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
    _swapChain = std::make_unique<SwapChain>(_window, _windowSurface.Internal());

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
    uint32_t currentFrame = ApplicationInfo::CurrentFrame();
    _waitFence.Get(currentFrame).Wait(UINT64_MAX);

    // Acquire image to draw on
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(ApplicationInfo::Device(), _swapChain->Internal(), UINT64_MAX, _availableImageSemaphore.Internal(currentFrame), VK_NULL_HANDLE, &imageIndex);
    ApplicationInfo::NextFrame();
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
    _waitFence.Get(currentFrame).Reset();

    UpdateCameraData();

    const CommandBuffer& commandBuffer = _commandBuffer.Get(currentFrame);

    commandBuffer.Reset();
    RecordCommandBuffer(commandBuffer, imageIndex);

    // stackallocs that can be cached.
    VkSemaphore wait[] = {_availableImageSemaphore.Internal(currentFrame)};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphore[] = {_renderFinishedSemaphores.Internal(imageIndex)};

    commandBuffer.Submit(wait, 1, waitStages, signalSemaphore, 1, _waitFence.Internal(currentFrame));

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

    result = vkQueuePresentKHR(_graphicsComputeQueue.Internal(), &presentInfo);

    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        RefreshSwapChain();
    }
    else if(result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    ApplicationInfo::NextFrame();
}

void Application::RecordCommandBuffer(const CommandBuffer& cmdBuffer, uint32_t imageIndex)
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
    const auto& meshRegistry = static_cast<MeshRegistry&>(*_registries[(size_t)RegistryType::Mesh]);
    const auto& materialRegistry = static_cast<MaterialRegistry&>(*_registries[(size_t)RegistryType::Material]);

    cmdBuffer.Begin();
    {
          // base param
        cmdBuffer.BeginMarker("Pipeline Init");
        {
            cmdBuffer.SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            cmdBuffer.SetPrimitiveRestart(false);
            cmdBuffer.SetRasterizerDiscard(false);
            cmdBuffer.SetDepthClampEnable(false);
            cmdBuffer.SetPolygonMode(VK_POLYGON_MODE_FILL);
            cmdBuffer.SetFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);
            cmdBuffer.SetDepthBiasEnable(false);
            cmdBuffer.SetLineWidth(1.0f);

            cmdBuffer.SetRasterizationSamples(ApplicationInfo::Constant::MaxMSAASample);
            cmdBuffer.SetSampleMask(ApplicationInfo::Constant::MaxMSAASample, 0xFFFFFFFF);
            cmdBuffer.SetAlphaToCoverageEnable(false);
            cmdBuffer.SetAlphaToOneEnable(false);

            ImageHelper::TransitionLayoutCommand(cmdBuffer, _colorBackBuffer->Internal(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            ImageHelper::TransitionLayoutCommand(cmdBuffer, _depthBuffer->Internal(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
            ImageHelper::TransitionLayoutCommand(cmdBuffer, backBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            cmdBuffer.SetViewport(0, 0, renderArea.extent.width, renderArea.extent.height);
            cmdBuffer.SetScissor(renderArea);

            for(const auto& registry : _registries)
            {
                registry->Bind(cmdBuffer, _descriptorAllocator.GlobalLayout());
            }
        }
        cmdBuffer.EndMarker();

        cmdBuffer.BindDescriptorSets(_descriptorAllocator.GlobalLayout(), _descriptorAllocator.GlobalDescriptorSet(), VK_PIPELINE_BIND_POINT_COMPUTE, 0);

        _cullingModule.Execute(cmdBuffer, _cameraDataBuffer, meshRegistry.IndirectBuffer(), meshRegistry.InstanceDataBuffer(), _descriptorAllocator.GlobalLayout());

        cmdBuffer.BeginMarker("Render Pass");
        {
            cmdBuffer.BindDescriptorSets(_descriptorAllocator.GlobalLayout(), _descriptorAllocator.GlobalDescriptorSet(), VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
            BufferIndexes indexes
            {
                .Cameras = _cameraDataBuffer.GetGPUIndex(),
                .VisibleInstanceIndirections = _cullingModule.VisibleInstanceIndirectionBuffer().GetGPUIndex(),
            };
            cmdBuffer.PushConstants(_descriptorAllocator.GlobalLayout(), 0, sizeof(BufferIndexes), &indexes);

            cmdBuffer.BeginRendering(renderArea, &colorAttachment, 1, depthAttachment, stencilAttachment);
            {
                _vertexShader.Bind(cmdBuffer);
                _fragmentShader.Bind(cmdBuffer);

                if(_wireframe)
                {
                    cmdBuffer.SetPolygonMode(VK_POLYGON_MODE_LINE);
                }

                const GraphicsBuffer& indirectBuffer = _cullingModule.VisibleIndirectBuffer();
                uint32_t drawCount = meshRegistry.IndirectBuffer().GetCurrentIndex();
                cmdBuffer.DrawIndexedIndirect(indirectBuffer.Internal(), 0, drawCount, indirectBuffer.GetStride());
            }
            cmdBuffer.EndRendering();
        }
        cmdBuffer.EndMarker();
        ImageHelper::TransitionLayoutCommand(cmdBuffer, backBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }
    cmdBuffer.End();
}

uint32_t Application::LoadDefaultMaterial()
{
    MaterialRegistry& materialRegistry = static_cast<MaterialRegistry&>(*_registries[(size_t)RegistryType::Material]);

    GPUAllocatedImage baseColorTex = GPUAllocatedImage(AssetHelper::ImageFromCPU(CPUImage("data/default/BaseColor.png", STBI_rgb_alpha), _graphicsCommandPool.Internal(), _graphicsComputeQueue.Internal()));
    uint32_t baseIndex = materialRegistry.RegisterTexture(_descriptorAllocator, std::move(baseColorTex), _sampler.Internal());

    GPUAllocatedImage normalMapTex = GPUAllocatedImage(AssetHelper::ImageFromCPU(CPUImage("data/default/BaseNormal.png", STBI_rgb_alpha), _graphicsCommandPool.Internal(), _graphicsComputeQueue.Internal()));
    uint32_t normalIndex = materialRegistry.RegisterTexture(_descriptorAllocator, std::move(normalMapTex), _sampler.Internal());

    GPUAllocatedImage mraoTex = GPUAllocatedImage(AssetHelper::ImageFromCPU(CPUImage("data/default/BaseMRAO.png", STBI_rgb_alpha), _graphicsCommandPool.Internal(), _graphicsComputeQueue.Internal()));
    uint32_t mraoIndex = materialRegistry.RegisterTexture(_descriptorAllocator, std::move(mraoTex), _sampler.Internal());

    MaterialProperties matProperties = MaterialProperties::Default();
    matProperties.BaseColorTexId = baseIndex;
    matProperties.NormalMapTexId = normalIndex;
    matProperties.MRAOTexId = mraoIndex;

    return materialRegistry.RegisterMaterial(matProperties);
}

void Application::LoadShaders()
{
    uint32_t vertex = _shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "Default.vert.spv", ShaderLibrary::Vertex, _descriptorAllocator);

    _vertexShader =
    {
        .Shader = _shaderLibrary.Get(vertex).Internal(),
        .State = VertexState()
    };

    uint32_t fragment = _shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "Default.frag.spv", ShaderLibrary::Fragment, _descriptorAllocator);

    _fragmentShader =
    {
        .Shader = _shaderLibrary.Get(fragment).Internal(),
        .State = FragmentState()
    };
}

Application::Application() :
    // Core Window & Instance
    _window(1920, 1080, "Window", this, Application::ResizeCallback, Application::MouseMoveCallback, Application::WindowFocusCallback),
    _instance(),
    _vkDebugLayer(_instance.Internal()),
    _windowSurface(_instance.Internal(), _window.Internal()),
    _physicalDevice(_instance.Internal(), _windowSurface.Internal(), _deviceExtensions),
    _device(_instance.Internal(), _windowSurface.Internal(), _deviceExtensions),

    // Queues & Swapchain
    _graphicsComputeQueue(VkQueueHandler::Graphics),
    _presentQueue(VkQueueHandler::Present),
    _swapChain(std::make_unique<SwapChain>(_window, _windowSurface.Internal())),
    _descriptorAllocator(),
    _registries(
        {
            std::make_unique<MeshRegistry>(_descriptorAllocator),
            std::make_unique<MaterialRegistry>(_descriptorAllocator),
        }),
    // Command pool
    _graphicsCommandPool(ApplicationInfo::GetGraphicsQueueId()),
    // Geometry & Data Buffers
    _colorBackBuffer(std::make_unique<GPUAllocatedImage>(_swapChain->GetExtent().width, _swapChain->GetExtent().height,
        ApplicationInfo::Get().GetMsaaSample(),
        1,
        _swapChain->GetImageFormat(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)),
    _depthBuffer(std::make_unique<GPUAllocatedImage>(_swapChain->GetExtent().width, _swapChain->GetExtent().height, ApplicationInfo::Get().GetMsaaSample(), 1, ApplicationInfo::Constant::DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)),

    _cameraDataBuffer(_descriptorAllocator, BINDING_CAMERA_BUFFER, GraphicsBuffer::STORAGE, RessourceUsage::PerFrame, 1, sizeof(CameraData)),
    _sampler(),
    _camera(glm::vec3(0.0f, 0.5f, 4.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 45.0f, (float)_swapChain->GetExtent().width / (float)_swapChain->GetExtent().height, 0.1f, 1000.0f),
    _commandBuffer(ApplicationInfo::Constant::MaxFrameInFlight, _graphicsCommandPool.Internal(), _graphicsComputeQueue.Internal()),

    // Synchronization
    _availableImageSemaphore(ApplicationInfo::Constant::MaxFrameInFlight),
    _renderFinishedSemaphores(_swapChain->Images.Size()),
    _waitFence(ApplicationInfo::Constant::MaxFrameInFlight),

    // modules
    _cullingModule(_shaderLibrary, _descriptorAllocator)
{
    _window.LockMouse(_mouseLocked);
    {
        CPUImage logo("data/logo.png", STBI_rgb_alpha);
        _window.SetIcon(logo.Data(), logo.Width(), logo.Height());
    }

    LoadShaders();

    MeshRegistry& meshRegistry = static_cast<MeshRegistry&>(*_registries[(size_t)RegistryType::Mesh]);
    MaterialRegistry& materialRegistry = static_cast<MaterialRegistry&>(*_registries[(size_t)RegistryType::Material]);

    uint32_t defaultMaterial = LoadDefaultMaterial();

    const float axisLength = 10.0f;
    const float axisThickness = 0.00625f;

    glm::vec3 aabbMin, aabbMax;
    uint32_t cube = MeshHelper::CubeMesh(meshRegistry, aabbMin, aabbMax);
    MaterialProperties defaultMat = materialRegistry.Material(defaultMaterial);

    {
        defaultMat.BaseColor = {1.0f, 0.0f, 0.0f, 1.0f};
        uint32_t matBinding = materialRegistry.RegisterMaterial(defaultMat);
        InstanceData instance = {
            .LocalToWorldMatrix = ApplicationHelper::TranslateRotateScale(glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(axisLength, axisThickness, axisThickness)),
            .AabbMin = aabbMin,
            .MaterialIndex = matBinding,
            .AabbMax = aabbMax,
            .MeshIndex = cube
        };
        meshRegistry.RegisterInstance(instance);
    }

    {
        defaultMat.BaseColor = {0.0f, 1.0f, 0.0f, 1.0f};
        uint32_t matBinding = materialRegistry.RegisterMaterial(defaultMat);
        InstanceData instance = {
            .LocalToWorldMatrix = ApplicationHelper::TranslateRotateScale(glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(axisThickness, axisLength, axisThickness)),
            .AabbMin = aabbMin,
            .MaterialIndex = matBinding,
            .AabbMax = aabbMax,
            .MeshIndex = cube
        };
        meshRegistry.RegisterInstance(instance);
    }

    {
        defaultMat.BaseColor = {0.0f, 0.0f, 1.0f, 1.0f};
        uint32_t matBinding = materialRegistry.RegisterMaterial(defaultMat);
        InstanceData instance = {
            .LocalToWorldMatrix = ApplicationHelper::TranslateRotateScale(glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(axisThickness, axisThickness, axisLength)),
            .AabbMin = aabbMin,
            .MaterialIndex = matBinding,
            .AabbMax = aabbMax,
            .MeshIndex = cube
        };
        meshRegistry.RegisterInstance(instance);
    }

    //AssetHelper::Load3DModel(_descriptorAllocator, "data/BistroExterior.m3vkasset", meshRegistry, materialRegistry, _graphicsCommandPool.Internal(), _graphicsComputeQueue.Internal(), _sampler.Internal());
    /*for(uint32_t i = 0; i < 100; ++i)
    {
        float red = (rand() / (float)RAND_MAX);
        float green = (rand() / (float)RAND_MAX);
        float blue = (rand() / (float)RAND_MAX);
        defaultMat.BaseColor = {red, green, blue, 1.0f};
        uint32_t matBinding = materialRegistry.RegisterMaterial(defaultMat);
    }

    float sample = 300000;
    float tr = 80.0f;

    for(uint32_t i = 0; i < sample; ++i)
    {
        uint32_t matBinding = rand() % materialRegistry.MaterialsCount();

        float x = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        float y = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        float z = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;

        float norm = sqrtf(x*x + y*y + z*z);

        float radius = (rand() / (float)RAND_MAX) * tr;

        x = (x / norm) * radius;
        y = (y / norm) * radius;
        z = (z / norm) * radius;

        float scale = radius / tr * 25.0f;
        InstanceData instance = {
            .LocalToWorldMatrix = ApplicationHelper::TranslateRotateScale(glm::vec3(x, y, z),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(axisThickness * scale, axisThickness * scale, axisThickness * scale)),
            .AabbMin = aabbMin,
            .MaterialIndex = matBinding,
            .AabbMax = aabbMax,
            .MeshIndex = cube
        };
        meshRegistry.RegisterInstance(instance);
    }*/

    for(auto& registry : _registries)
    {
        registry->UploadAndRelease(_graphicsComputeQueue.Internal(), _graphicsCommandPool.Internal());
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
            if(_window.IsKeyPressed(GLFW_KEY_Z)) _wireframe = !_wireframe; _inputPrevent = ApplicationInfo::Constant::InputPrevent;
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
    _swapChain.reset();
}

void Application::Run()
{
    MainLoop();
}
