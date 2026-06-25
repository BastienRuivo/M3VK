#include "application/Application.h"
#include "Material.h"
#include "application/ApplicationHelper.h"
#include "asset/AssetImporter.h"
#include "asset/MeshHelper.h"
#include "glm/matrix.hpp"
#include "imgui.h"
#include "modules/DrawModule.h"
#include "rendering/DescriptorAllocator.h"
#include "rendering/GraphicsBuffer.h"
#include "application/ApplicationInfo.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GPUImage.h"
#include "rendering/GraphicsBuffer.h"
#include "registry/MaterialRegistry.h"
#include "registry/MeshRegistry.h"
#include "rendering/GraphicsImage.h"
#include "rendering/ImageHelper.h"
#include "rendering/MultiFrame.h"
#include "registry/Registry.h"
#include "rendering/RessourceUsage.h"
#include "rendering/Shaders/ShaderLibrary.h"
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

#include "Camera.h"

#include "ShaderBindings.h"

#ifdef M3VK_VERBOSE_LOG
#include <string>
#endif

#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void ExtractFrusumPlane(CameraData& data)
{
    glm::mat4 viewProjectionMatrix = glm::transpose(data.ViewProjectionMatrix);

    glm::vec4 left = viewProjectionMatrix[3] + viewProjectionMatrix[0];
    data.FrustumPlanes[CameraData::Planes::Left].Normal = glm::vec3(left);
    data.FrustumPlanes[CameraData::Planes::Left].Distance = left.w;

    glm::vec4 right = viewProjectionMatrix[3] - viewProjectionMatrix[0];
    data.FrustumPlanes[CameraData::Planes::Right].Normal = glm::vec3(right);
    data.FrustumPlanes[CameraData::Planes::Right].Distance = right.w;

    glm::vec4 top = viewProjectionMatrix[3] - viewProjectionMatrix[1];
    data.FrustumPlanes[CameraData::Planes::Top].Normal = glm::vec3(top);
    data.FrustumPlanes[CameraData::Planes::Top].Distance = top.w;

    glm::vec4 bottom = viewProjectionMatrix[3] + viewProjectionMatrix[1];
    data.FrustumPlanes[CameraData::Planes::Bottom].Normal = glm::vec3(bottom);
    data.FrustumPlanes[CameraData::Planes::Bottom].Distance = bottom.w;

    glm::vec4 near = viewProjectionMatrix[3] + viewProjectionMatrix[2];
    data.FrustumPlanes[CameraData::Planes::Near].Normal = glm::vec3(near);
    data.FrustumPlanes[CameraData::Planes::Near].Distance = near.w;

    glm::vec4 far = viewProjectionMatrix[3] - viewProjectionMatrix[2];
    data.FrustumPlanes[CameraData::Planes::Far].Normal = glm::vec3(far);
    data.FrustumPlanes[CameraData::Planes::Far].Distance = far.w;

    for (int i = 0; i < 6; i++)
    {
        data.FrustumPlanes[i].Normalize();
    }
}


void Application::UpdateCameraData()
{
    VkExtent2D extent = _swapChain->GetExtent();

    CameraData cameraData =
    {
        .WorldToCameraMatrix = _camera.GetViewMatrix(),
        .ProjectionMatrix = _camera.GetProjectionMatrix()
    };
    // it was designed for opengl so flip it
    cameraData.ProjectionMatrix[1][1] *= -1;

    cameraData.ViewProjectionMatrix = cameraData.ProjectionMatrix * cameraData.WorldToCameraMatrix;

    cameraData.InverseViewProjectionMatrixNoTranslation = glm::inverse(glm::mat4(glm::mat3(cameraData.ViewProjectionMatrix)));
    cameraData.InverseViewProjectionMatrix = glm::inverse(cameraData.ViewProjectionMatrix);

    ExtractFrusumPlane(cameraData);

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

    if(!app->_mouseLocked)
    {
        dx = dy = 0.0f;
    }

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

    _msaaColorTarget.reset();
    _msaaColorTarget = MakeMsaaColorTarget();

    _msaaDepthTarget.reset();
    _msaaDepthTarget = MakeDepthTarget();
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

    _UserInterface.StartFrame();
    ImGui::Begin("Parameters");
    ImGui::Checkbox("Wireframe", &_wireframe);
    if(ImGui::BeginCombo("Draw Debug Mode", DebugModeNames[_debug]))
    {
        for (int n = 0; n < DebugMode::DB_Count; n++)
        {
            // Track if the current item in the loop is the selected one
            const bool isSelected = (_debug == n);

            if (ImGui::Selectable(DebugModeNames[n], isSelected))
            {
                DebugMode newVal = static_cast<DebugMode>(n);

                if((newVal == DebugMode::DB_None && _debug  != DebugMode::DB_None) || (newVal != DebugMode::DB_None && _debug  == DebugMode::DB_None))
                {
                    vkDeviceWaitIdle(ApplicationInfo::Device());
                    for(auto& module : _drawModules)
                    {
                        _shaderLibrary.DisposeShader(module.VertexBinding.LibraryIndex);
                        _shaderLibrary.DisposeShader(module.FragmentBinding.LibraryIndex);
                    }
                    _drawModules = InitDrawModule(newVal != DebugMode::DB_None);
                }
                _debug = newVal;
            }

            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    if(ImGui::BeginCombo("Image Viewer", ImageVizualizationModeNames[_imageVizualizationMode]))
    {
        for (int n = 0; n < ImageVizualizationMode::IMV_Count; n++)
        {
            // Track if the current item in the loop is the selected one
            const bool isSelected = (_imageVizualizationMode == n);

            if (ImGui::Selectable(ImageVizualizationModeNames[n], isSelected))
            {
                ImageVizualizationMode newVal = static_cast<ImageVizualizationMode>(n);
                _imageVizualizationMode = newVal;
            }

            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if(_imageVizualizationMode == ImageVizualizationMode::IMV_BackBuffer)
    {
        ImGui::Image((ImTextureID) _msaaColorTarget->ImGuiSet(), ImVec2(500, 500), ImVec2(0, 1), ImVec2(1, 0));
    }
    else if(_imageVizualizationMode == ImageVizualizationMode::IMV_DepthBuffer)
    {
        ImGui::Image((ImTextureID) _msaaDepthTarget->ImGuiSet(), ImVec2(500, 500), ImVec2(0, 1), ImVec2(1, 0));
    }

    ImGui::End();

    _UserInterface.Render();
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

    VkRenderingAttachmentInfo colorAttachment = ImageHelper::AttachmentInfo(_msaaColorTarget->View(),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        _swapChain->View(imageIndex),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        {{}});

    VkRenderingAttachmentInfo depthAttachment = ImageHelper::AttachmentInfo(_msaaDepthTarget->View(), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, {1.0f, 0.0});
    // there is no stencil buffer
    VkRenderingAttachmentInfo stencilAttachment { .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };

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

            ImageHelper::TransitionLayoutCommand(cmdBuffer, _msaaColorTarget->Internal(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            ImageHelper::TransitionLayoutCommand(cmdBuffer, _msaaDepthTarget->Internal(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
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
            CommonIndexes indexes
            {
                .DebugIndex = _debug,
                .Cameras = _cameraDataBuffer.GetGPUIndex(),
                .VisibleInstanceIndirections = _cullingModule.VisibleInstanceIndirectionBuffer().GetGPUIndex(),
            };
            cmdBuffer.PushConstants(_descriptorAllocator.GlobalLayout(), 0, sizeof(CommonIndexes), &indexes);

            cmdBuffer.BeginRendering(renderArea, &colorAttachment, 1, depthAttachment, stencilAttachment);
            {
                for(uint32_t i = 0; i < _drawModules.size(); i++)
                {
                    MeshRegistry::LayerMaterialInfo drawInfo = meshRegistry.GetIndirectDrawInfo(static_cast<MaterialType>(i));
                    _drawModules[i].Execute(cmdBuffer, _descriptorAllocator.GlobalLayout(), _cullingModule.VisibleIndirectBuffer(), drawInfo.offset, drawInfo.count, _wireframe);
                }

                _skyboxModule.Execute(cmdBuffer, _descriptorAllocator.GlobalLayout());
            }
            cmdBuffer.EndRendering();

            if(_imageVizualizationMode == ImageVizualizationMode::IMV_DepthBuffer)
            {
                ImageHelper::TransitionLayoutCommand(cmdBuffer,_msaaDepthTarget->Internal(), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);
            }

            VkRenderingAttachmentInfo uiColorAttachment = ImageHelper::AttachmentInfo(backBuffer.View, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);

            VkRenderingAttachmentInfo uiDepthAttachment { .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };

            cmdBuffer.BeginRendering(renderArea, &uiColorAttachment, 1, uiDepthAttachment, stencilAttachment);
            {
                _UserInterface.Draw(cmdBuffer.GetInternal());
            }
            cmdBuffer.EndRendering();
        }
        cmdBuffer.EndMarker();
        ImageHelper::TransitionLayoutCommand(cmdBuffer, backBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }
    cmdBuffer.End();
}

std::array<DrawModule, MaterialType::Count> Application::InitDrawModule(bool DebugOn)
{
    std::vector<Shader::SpecializationConstant> constants
    {
        Shader::SpecializationConstant{
            .name = "ENABLE_DEBUG",
            .enabled = DebugOn
        }
    };
    uint32_t sVertex = _shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "Draw.vert.spv", ShaderLibrary::Vertex, _descriptorAllocator, constants);
    uint32_t sFragmentOpaque = _shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "Draw.frag.spv", ShaderLibrary::Fragment, _descriptorAllocator, constants);
    uint32_t sFragmentCutout = _shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "DrawAlphaCutout.frag.spv", ShaderLibrary::Fragment, _descriptorAllocator, constants);

    ShaderLibrary::VertexBinding vertexCullBack =
    {
        .LibraryIndex = sVertex,
        .Handle = _shaderLibrary.GetHandle(sVertex),
        .State = VertexState()
    };

    ShaderLibrary::VertexBinding vertexCullOff =
    {
        .LibraryIndex = vertexCullBack.LibraryIndex,
        .Handle = vertexCullBack.Handle,
        .State = VertexState()
    };
    vertexCullOff.State.CullMode = VK_CULL_MODE_NONE;


    ShaderLibrary::FragmentBinding fragmentOpaque =
    {
        .LibraryIndex = sFragmentOpaque,
        .Handle = _shaderLibrary.GetHandle(sFragmentOpaque),
        .State = FragmentState()
    };

    ShaderLibrary::FragmentBinding fragmentCutout =
    {
        .LibraryIndex = sFragmentCutout,
        .Handle = _shaderLibrary.GetHandle(sFragmentCutout),
        .State = fragmentOpaque.State
    };


    return std::array<DrawModule, MaterialType::Count>
    {
        DrawModule(vertexCullBack, fragmentOpaque),
        DrawModule(vertexCullBack, fragmentCutout),
        DrawModule(vertexCullOff, fragmentCutout),
        DrawModule(vertexCullBack, fragmentOpaque)
    };
}

std::unique_ptr<GraphicsImage> Application::MakeMsaaColorTarget() const
{
    return std::make_unique<GraphicsImage>(_samplerLinear.Internal(), RessourceUsage::Static,
        _swapChain->GetExtent().width, _swapChain->GetExtent().height,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        _swapChain->GetImageFormat(),
        1, VK_IMAGE_TILING_OPTIMAL, ApplicationInfo::GetMsaaSample());
}

std::unique_ptr<GraphicsImage> Application::MakeDepthTarget() const
{
    return std::make_unique<GraphicsImage>(_samplerNearest.Internal(), RessourceUsage::Static,
        _swapChain->GetExtent().width, _swapChain->GetExtent().height,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        ApplicationInfo::Constant::DepthFormat, 1, VK_IMAGE_TILING_OPTIMAL, ApplicationInfo::GetMsaaSample());
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
            std::make_unique<MaterialRegistry>(_descriptorAllocator, ApplicationInfo::Constant::MaxMaterialTextureCount, _graphicsCommandPool.Internal(), _graphicsComputeQueue.Internal(), _samplerLinear.Internal())
        }),
    // Command pool
    _graphicsCommandPool(ApplicationInfo::GetGraphicsQueueId()),
    // Geometry & Data Buffers
    _msaaColorTarget(MakeMsaaColorTarget()),
    _msaaDepthTarget(MakeDepthTarget()),

    _cameraDataBuffer(_descriptorAllocator, BINDING_CAMERA_BUFFER, GraphicsBuffer::STORAGE, RessourceUsage::PerFrame, 1, sizeof(CameraData)),
    _samplerLinear(),
    _samplerNearest(VK_FILTER_NEAREST, VK_FILTER_NEAREST),
    _camera(glm::vec3(0.0f, 0.5f, 4.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 45.0f, (float)_swapChain->GetExtent().width / (float)_swapChain->GetExtent().height, 0.1f, 1000.0f),
    _commandBuffer(ApplicationInfo::Constant::MaxFrameInFlight, _graphicsCommandPool.Internal(), _graphicsComputeQueue.Internal()),

    // Synchronization
    _availableImageSemaphore(ApplicationInfo::Constant::MaxFrameInFlight),
    _renderFinishedSemaphores(_swapChain->Images.Size()),
    _waitFence(ApplicationInfo::Constant::MaxFrameInFlight),

    _UserInterface(_window.Internal(), *_swapChain, _graphicsComputeQueue.Internal(), _graphicsCommandPool.Internal()),

    _drawModules(InitDrawModule(false)),
    _skyboxModule(_shaderLibrary, _descriptorAllocator, _graphicsCommandPool.Internal(), _graphicsComputeQueue.Internal()),

    // modules
    _cullingModule(_shaderLibrary, _descriptorAllocator)
{
    _window.LockMouse(_mouseLocked);
    {
        CPUImage logo("data/logo.png", STBI_rgb_alpha);
        _window.SetIcon(logo.Data(), logo.Width(), logo.Height());
    }

    MeshRegistry& meshRegistry = static_cast<MeshRegistry&>(*_registries[(size_t)RegistryType::Mesh]);
    MaterialRegistry& materialRegistry = static_cast<MaterialRegistry&>(*_registries[(size_t)RegistryType::Material]);

    const float axisLength = 10.0f;
    const float axisThickness = 0.00625f;

    glm::vec3 aabbMin, aabbMax;
    uint32_t cube = MeshHelper::CubeMesh(meshRegistry, MaterialType::Opaque, aabbMin, aabbMax);
    MaterialProperties defaultMat = materialRegistry.Material(materialRegistry.DefaultMaterialIndex());

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
        meshRegistry.RegisterInstance(MaterialType::Opaque, instance);
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
        meshRegistry.RegisterInstance(MaterialType::Opaque, instance);
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
        meshRegistry.RegisterInstance(MaterialType::Opaque, instance);
    }


    InstanceData instance = {
        .LocalToWorldMatrix = ApplicationHelper::TranslateRotateScale(glm::vec3(0.0f, 10.0f, 10.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1, 1, 1)),
        .AabbMin = aabbMin,
        .MaterialIndex = 1,
        .AabbMax = aabbMax,
        .MeshIndex = cube
    };

    meshRegistry.RegisterInstance(MaterialType::Opaque, instance);

   AssetImporter::LoadAsset(_descriptorAllocator, "data/BistroExterior.m3vkasset", meshRegistry, materialRegistry, _graphicsCommandPool.Internal(), _graphicsComputeQueue.Internal(), _samplerLinear.Internal());
   //ImporterHelper::Load3DModel(_descriptorAllocator, "data/BistroInterior_Wine.m3vkasset", meshRegistry, materialRegistry, _graphicsCommandPool.Internal(), _graphicsComputeQueue.Internal(), _samplerLinear.Internal());
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
        meshRegistry.RegisterInstance(MaterialType::Opaque, instance);
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
            if(_window.IsKeyPressed(GLFW_KEY_LEFT_ALT)) _window.LockMouse(_mouseLocked = !_mouseLocked); _inputPrevent = glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate;
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
