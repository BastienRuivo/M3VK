#include "application/Pipeline.h"
#include "Camera.h"
#include "ShaderBindings.h"
#include "allocation/BindingManager.h"
#include "application/ApplicationHelper.h"
#include "application/ApplicationInfo.h"
#include "application/UserInterface.h"
#include "application/Window.h"
#include "asset/AssetImporter.h"
#include "asset/MeshHelper.h"
#include "allocation/MaterialRegistry.h"
#include "allocation/MeshRegistry.h"
#include "rendering/CommandBuffer.h"
#include "rendering/SwapChain.h"
#include <cstring>
#include <vulkan/vulkan_core.h>

Pipeline::Pipeline(const SwapChain& swapChain, VkCommandPool graphicsCommandPool, VkQueue graphicsComputeQueue)
: _bindingManager(),
    _samplerLinear(),
    _samplerNearest(VK_FILTER_NEAREST, VK_FILTER_NEAREST),
    _registries(
        {
            std::make_unique<MeshRegistry>(_bindingManager),
            std::make_unique<MaterialRegistry>(_bindingManager, graphicsCommandPool, graphicsComputeQueue, _samplerLinear.Internal())
        }),
    _cameraDataBuffer(_bindingManager, BINDING_CAMERA_BUFFER, GraphicsBuffer::STORAGE, RessourceUsage::PerFrame, 1, sizeof(CameraData)),
    _camera(glm::vec3(-14.0f, 0.5f, 0.0f), glm::vec3(10, 0, 0), 45.0f, (float)swapChain.GetExtent().width / (float)swapChain.GetExtent().height),
    _msaaColorTarget(RessourceUsage::Transient, graphicsCommandPool, graphicsComputeQueue,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        swapChain.GetExtent().width, swapChain.GetExtent().height,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        swapChain.GetImageFormat(),
        1, VK_IMAGE_TILING_OPTIMAL, ApplicationInfo::GetMsaaSample()),
    _msaaDepthTarget(RessourceUsage::Transient, graphicsCommandPool, graphicsComputeQueue,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        swapChain.GetExtent().width, swapChain.GetExtent().height,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
        ApplicationInfo::Constant::DepthFormat, 1, VK_IMAGE_TILING_OPTIMAL, ApplicationInfo::GetMsaaSample()),
    _finalColorTarget(BindlessTexture::Register(_bindingManager, RessourceUsage::PerFrame, _samplerNearest.Internal(), graphicsCommandPool, graphicsComputeQueue,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        swapChain.GetExtent().width, swapChain.GetExtent().height,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        swapChain.GetImageFormat(),
        1)),
    _finalDepthTarget(BindlessTexture::Register(_bindingManager,RessourceUsage::PerFrame, _samplerNearest.Internal(), graphicsCommandPool, graphicsComputeQueue,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        swapChain.GetExtent().width, swapChain.GetExtent().height,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        ApplicationInfo::Constant::DepthFormat,
        1)),
        _drawModules({}),
        _drawTextureDebug(_shaderLibrary, _bindingManager)
{
    InitModules(swapChain, graphicsCommandPool, graphicsComputeQueue);
    MeshRegistry& meshRegistry = static_cast<MeshRegistry&>(*_registries[(size_t)RegistryType::Mesh]);
    MaterialRegistry& materialRegistry = static_cast<MaterialRegistry&>(*_registries[(size_t)RegistryType::Material]);

    /*const float axisLength = 10.0f;
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
        //meshRegistry.RegisterInstance(MaterialType::Opaque, instance);
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
        //meshRegistry.RegisterInstance(MaterialType::Opaque, instance);
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
        //meshRegistry.RegisterInstance(MaterialType::Opaque, instance);
    }


    InstanceData instance = {
        .LocalToWorldMatrix = ApplicationHelper::TranslateRotateScale(glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.1, 10, 10)),
        .AabbMin = aabbMin,
        .MaterialIndex = 1,
        .AabbMax = aabbMax,
        .MeshIndex = cube
    };

    meshRegistry.RegisterInstance(MaterialType::Opaque, instance);

    instance = {
        .LocalToWorldMatrix = ApplicationHelper::TranslateRotateScale(glm::vec3(4.0f, 0.0f, 1.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1, 1, 1)),
        .AabbMin = aabbMin,
        .MaterialIndex = 2,
        .AabbMax = aabbMax,
        .MeshIndex = cube
    };
    meshRegistry.RegisterInstance(MaterialType::Opaque, instance);*/

    AssetImporter::LoadAsset(_bindingManager, "data/BistroExterior.m3vkasset", meshRegistry, materialRegistry, graphicsCommandPool, graphicsComputeQueue, _samplerLinear.Internal());
    //ImporterHelper::Load3DModel(_descriptorAllocator, "data/BistroInterior_Wine.m3vkasset", meshRegistry, materialRegistry, _graphicsCommandPool.Internal(), _graphicsComputeQueue.Internal(), _samplerLinear.Internal());

    for(auto& registry : _registries)
    {
        registry->UploadAndRelease(graphicsComputeQueue, graphicsCommandPool);
    }
}

Pipeline::~Pipeline()
{

}

void Pipeline::RefreshDrawModule(MaterialType type, const ShaderLibrary::VertexBinding& vertex, const ShaderLibrary::FragmentBinding& frag)
{
    auto drawModule = std::make_unique<DrawModule>(vertex, frag);
    drawModule->ModuleId = _modules.size();

    if(_drawModules[type] == nullptr)
    {
        _drawModules[type] = drawModule.get();
        _modules.push_back(std::move(drawModule));
    }
    else
    {
        uint32_t index = _drawModules[type]->ModuleId;
        _drawModules[type] = drawModule.get();
        drawModule->ModuleId = index;
        _modules[index] = std::move(drawModule);
    }
}

void Pipeline::InitDrawModules(bool DebugOn)
{
     std::vector<Shader::SpecializationConstant> constants
    {
        Shader::SpecializationConstant{
            .name = "ENABLE_DEBUG",
            .enabled = DebugOn
        }
    };
    uint32_t sVertex = _shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "Draw.vert.spv", ShaderLibrary::Vertex, _bindingManager, constants);
    uint32_t sFragmentOpaque = _shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "Draw.frag.spv", ShaderLibrary::Fragment, _bindingManager, constants);
    uint32_t sFragmentCutout = _shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "DrawAlphaCutout.frag.spv", ShaderLibrary::Fragment, _bindingManager, constants);

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

    RefreshDrawModule(MaterialType::Opaque, vertexCullBack, fragmentOpaque);
    RefreshDrawModule(MaterialType::Cutout, vertexCullBack, fragmentCutout);
    RefreshDrawModule(MaterialType::CutoutTwoSided, vertexCullOff, fragmentCutout);

    // TODO Transparent handling
    RefreshDrawModule(MaterialType::Transparent, vertexCullBack, fragmentOpaque);
}

void Pipeline::InitModules(const SwapChain& swapChain, VkCommandPool pool, VkQueue queue)
{
    InitDrawModules(false);
    _modules.push_back(std::make_unique<SkyboxModule>(_shaderLibrary, _bindingManager, pool, queue));
    _skyboxModule = static_cast<SkyboxModule*>(_modules.back().get());
    _modules.push_back(std::make_unique<CullingModule>(_shaderLibrary, _bindingManager));
    _cullingModule = static_cast<CullingModule*>(_modules.back().get());
    _modules.push_back(std::make_unique<HiZGenerateModule>(swapChain, _shaderLibrary, _bindingManager, pool, queue, _samplerNearest.Internal()));
    _hizGenerateModule = static_cast<HiZGenerateModule*>(_modules.back().get());
}

void Pipeline::OnMouseMove(float dx, float dy)
{
    _camera.Rotate(dx, dy);
}

void Pipeline::FrameInit(const CommandBuffer& cmdBuffer, VkRect2D renderArea) const
{
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

        cmdBuffer.SetViewport(0, 0, renderArea.extent.width, renderArea.extent.height);
        cmdBuffer.SetScissor(renderArea);

        for(const auto& registry : _registries)
        {
            registry->Bind(cmdBuffer, _bindingManager.GlobalLayout());
        }
    }
    cmdBuffer.EndMarker();
}

void Pipeline::Execute(const CommandBuffer& cmdBuffer, const SwapChain& swapChain, const UserInterface& ui, uint32_t imageIndex)
{
    const auto & finalColorTarget = _finalColorTarget.Texture(_bindingManager).Internal();
    const auto & finalDepthTarget = _finalDepthTarget.Texture(_bindingManager).Internal();

    VkRect2D renderArea = {0, 0, swapChain.GetExtent().width, swapChain.GetExtent().height};

    VkRenderingAttachmentInfo colorAttachment = ImageHelper::AttachmentInfo(_msaaColorTarget.View(),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        finalColorTarget.View,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        {{}});

    VkRenderingAttachmentInfo depthAttachment = ImageHelper::AttachmentInfo(_msaaDepthTarget.View(),
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        finalDepthTarget.View,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        {.depthStencil = {1.0f}});

    // there is no stencil buffer
    VkRenderingAttachmentInfo stencilAttachment { .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };

    const ImageReference& backBuffer = swapChain.Images.Get(imageIndex);
    const auto& meshRegistry = static_cast<MeshRegistry&>(*_registries[(size_t)RegistryType::Mesh]);
    const auto& materialRegistry = static_cast<MaterialRegistry&>(*_registries[(size_t)RegistryType::Material]);

    cmdBuffer.Begin();
    {
        FrameInit(cmdBuffer, renderArea);

        ImageHelper::TransitionLayoutCommand(cmdBuffer, finalColorTarget, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        ImageHelper::TransitionLayoutCommand(cmdBuffer, backBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        GlobalConstants indexes
        {
            .DebugIndex = _debug,
            .Cameras = _cameraDataBuffer.GetGPUIndex(),
            .VisibleInstanceIndirections = _cullingModule->VisibleInstanceIndirectionBuffer().GetGPUIndex(),
            .VisibleDrawIndirects = _cullingModule->VisibleIndirectBuffer().GetGPUIndex(),
            .HizIndex = _hizGenerateModule->HiZTexture().PreviousBindlessIndex(),
            .ScreenSize = glm::uvec2(backBuffer.Width, backBuffer.Height),
            .ScreenPixelSize = glm::vec2(1.0f / backBuffer.Width, 1.0f/ backBuffer.Height)
        };
        cmdBuffer.PushConstants(_bindingManager.GlobalLayout(), 0, COMMON_INDEXES_OFFSET, &indexes);

        cmdBuffer.BeginMarker("Culling Compute Pass");
        {
            cmdBuffer.BindDescriptorSets(_bindingManager.GlobalLayout(), _bindingManager.GlobalDescriptorSet(), VK_PIPELINE_BIND_POINT_COMPUTE, 0);
            _cullingModule->Execute(cmdBuffer, _hizGenerateModule->HiZTexture().PreviousBindlessIndex(), _cameraDataBuffer, meshRegistry.IndirectBuffer(), meshRegistry.InstanceDataBuffer(), _bindingManager.GlobalLayout());
        }
        cmdBuffer.EndMarker();

        cmdBuffer.BeginMarker("Render Pass");
        {
            cmdBuffer.BindDescriptorSets(_bindingManager.GlobalLayout(), _bindingManager.GlobalDescriptorSet(), VK_PIPELINE_BIND_POINT_GRAPHICS, 0);

            cmdBuffer.BeginRendering(renderArea,  {&colorAttachment, 1}, depthAttachment, stencilAttachment);
            {
                if(_wireframe)
                {
                    cmdBuffer.SetPolygonMode(VK_POLYGON_MODE_LINE);
                }
                for(uint32_t i = 0; i < MaterialType::Count; ++i)
                {
                    auto drawInfo = meshRegistry.GetIndirectDrawInfo(static_cast<MaterialType>(i));
                    _drawModules[i]->Execute(cmdBuffer, _bindingManager.GlobalLayout(), _cullingModule->VisibleIndirectBuffer(), drawInfo.offset, drawInfo.count);
                }

                _skyboxModule->Execute(cmdBuffer, _bindingManager.GlobalLayout());
            }
            cmdBuffer.EndRendering();
            cmdBuffer.SetRasterizationSamples(VK_SAMPLE_COUNT_1_BIT);
        }
        cmdBuffer.EndMarker();

        _hizGenerateModule->Execute(cmdBuffer, _bindingManager, _finalDepthTarget, _bindingManager.GlobalLayout());

        cmdBuffer.BeginMarker("Copy To Back Buffer");
        {
            ImageHelper::TransitionLayoutCommand(cmdBuffer, finalColorTarget, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            cmdBuffer.CopyImage(finalColorTarget, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, backBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            ImageHelper::TransitionLayoutCommand(cmdBuffer, backBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }
        cmdBuffer.EndMarker();

        cmdBuffer.BeginMarker("Render UI");
        {
            VkRenderingAttachmentInfo uiColorAttachment = ImageHelper::AttachmentInfo(backBuffer.View, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);
            VkRenderingAttachmentInfo uiDepthAttachment { .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
            cmdBuffer.BeginRendering(renderArea, {&uiColorAttachment, 1}, uiDepthAttachment, stencilAttachment);
            {
                for(const auto & module : _modules)
                {
                    module->RenderUI(cmdBuffer, _drawTextureDebug, _bindingManager.GlobalLayout());
                }
                ui.Draw(cmdBuffer.GetInternal());
            }
            cmdBuffer.EndRendering();
        }
        cmdBuffer.EndMarker();
        ImageHelper::TransitionLayoutCommand(cmdBuffer, backBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }
    cmdBuffer.End();
}

void Pipeline::PreRender(VkExtent2D size)
{
    UpdateCameraData(size);
}

void Pipeline::Refresh(const CommandBuffer& cmdBuffer, VkExtent2D newSize)
{
    _camera._aspect = static_cast<float>(newSize.width) / static_cast<float>(newSize.height);

    if(_msaaColorTarget.Resize(newSize.width, newSize.height))
    {
        ImageHelper::TransitionLayoutCommand(cmdBuffer, _msaaColorTarget.Internal(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }
    if(_msaaDepthTarget.Resize(newSize.width, newSize.height))
    {
    ImageHelper::TransitionLayoutCommand(cmdBuffer, _msaaDepthTarget.Internal(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }
    if(_finalColorTarget.Resize(_bindingManager, newSize.width, newSize.height))
    {
        _finalColorTarget.TransistionAllLayoutCommand(_bindingManager, cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    }
    if(_finalDepthTarget.Resize(_bindingManager, newSize.width, newSize.height))
    {
        _finalDepthTarget.TransistionAllLayoutCommand(_bindingManager, cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    }
    for(auto & module : _modules)
    {
        module->Resize(cmdBuffer, _bindingManager, newSize.width, newSize.height);
    }
}

void Pipeline::DoKeyboardInput(const Window& window, float deltaTime)
{
    float speed = _camera.speed * deltaTime;

    if(window.IsKeyPressed(GLFW_KEY_LEFT_SHIFT)) speed *= 10.0;
    if(window.IsKeyPressed(GLFW_KEY_LEFT_CONTROL)) speed /= 10.0;

    if(window.IsKeyPressed(GLFW_KEY_W)) _camera.position += speed * _camera.Front();
    if(window.IsKeyPressed(GLFW_KEY_S)) _camera.position -= speed * _camera.Front();
    if(window.IsKeyPressed(GLFW_KEY_A)) _camera.position -= speed * glm::normalize(glm::cross(_camera.Front(), _camera.Up()));
    if(window.IsKeyPressed(GLFW_KEY_D)) _camera.position += speed * glm::normalize(glm::cross(_camera.Front(), _camera.Up()));
    if(window.IsKeyPressed(GLFW_KEY_E)) _camera.position += speed * _camera.Up();
    if(window.IsKeyPressed(GLFW_KEY_Q)) _camera.position -= speed * _camera.Up();
}

void Pipeline::DoUI(const UserInterface& ui)
{
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
                        _shaderLibrary.DisposeShader(module->VertexBinding.LibraryIndex);
                        _shaderLibrary.DisposeShader(module->FragmentBinding.LibraryIndex);
                    }
                    InitDrawModules(newVal != DebugMode::DB_None);
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

    double currentSize = (double)ApplicationInfo::GetVRAMUsage() / 1048576; //(MB)

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Text("VRAM Usage = %.3f MB", currentSize);

    ImGui::End();

    for(auto& module : _modules)
    {
        module->DoUI(ui);
    }
}

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

void Pipeline::UpdateCameraData(VkExtent2D extent)
{
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
