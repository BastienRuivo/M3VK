#pragma once

#include "Material.h"
#include "application/UserInterface.h"
#include "application/Window.h"
#include "modules/CullingModule.h"
#include "modules/DrawModule.h"
#include "modules/HiZGenerateModule.h"
#include "modules/SkyboxModule.h"
#include "allocation/Registry.h"
#include "rendering/Camera.h"
#include "allocation/BindingManager.h"
#include "rendering/GraphicsImage.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include "rendering/SwapChain.h"
#include <vulkan/vulkan_core.h>
class Pipeline
{
    public:
    Pipeline(const SwapChain& swapChain, VkCommandPool graphicsCommandPool, VkQueue graphicsComputeQueue);
    ~Pipeline();

    void PreRender(VkExtent2D size);
    void Refresh(const CommandBuffer& cmdBuffer, VkExtent2D size);

    void DoUI(const UserInterface& ui);
    void DoKeyboardInput(const Window& window, float deltaTime);

    void UpdateCameraData(VkExtent2D size);
    void Execute(const CommandBuffer& cmdBuffer, const SwapChain& swapChain, const UserInterface& ui, uint32_t imageIndex);
    void OnMouseMove(float dx, float dy);

    protected:
    void InitModules(const SwapChain& swapChain, VkCommandPool pool, VkQueue queue);

    void RefreshDrawModule(MaterialType type, const ShaderLibrary::VertexBinding& vertex, const ShaderLibrary::FragmentBinding& frag) ;
    void InitDrawModules(bool DebugOn);
    void FrameInit(const CommandBuffer& cmdBuffer, VkRect2D renderArea) const;

    BindingManager _bindingManager;
    VkSamplerHandler _samplerLinear;
    VkSamplerHandler _samplerNearest;
    ShaderLibrary _shaderLibrary;
    std::array<std::unique_ptr<Registry>, 2> _registries;
    enum class RegistryType
    {
        Mesh = 0,
        Material = 1
    };

    // Ressources
    // Pipeline
    GraphicsImage _msaaColorTarget;
    GraphicsImage _msaaDepthTarget;
    BindingManager::BindlessTexture _finalColorTarget;
    BindingManager::BindlessTexture _finalDepthTarget;

    GraphicsBuffer _cameraDataBuffer;
    Camera _camera;

    // Modules
    std::vector<std::unique_ptr<Module>> _modules;

    CullingModule* _cullingModule;
    std::array<DrawModule*, MaterialType::Count> _drawModules;
    SkyboxModule* _skyboxModule;
    HiZGenerateModule* _hizGenerateModule;

    FullscreenDrawDebug _drawTextureDebug;

    // debug
    bool _wireframe = false;

    enum DebugMode
    {
        DB_None,
        DB_VertexNormal,
        DB_Normal,
        DB_Count
    };
    DebugMode _debug = DebugMode::DB_None;
    const char* DebugModeNames[DebugMode::DB_Count] =
    {
        "None",
        "VertexNormal",
        "Normal"
    };

    enum ImageVizualizationMode
    {
        IMV_None,
        IMV_BackBuffer,
        IMV_DepthBuffer,
        IMV_Count
    };
    ImageVizualizationMode _imageVizualizationMode = ImageVizualizationMode::IMV_None;
    const char* ImageVizualizationModeNames[ImageVizualizationMode::IMV_Count] =
    {
        "None",
        "BackBuffer",
        "DepthBuffer"
    };
};
