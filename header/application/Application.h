#pragma once

#include "application/UserInterface.h"
#include "modules/CullingModule.h"
#include "modules/DrawModule.h"
#include "rendering/Camera.h"
#include "rendering/DescriptorAllocator.h"
#include "registry/Registry.h"
#include "rendering/CommandBuffer.h"
#include "rendering/MultiFrame.h"
#include "handler/Handlers.h"
#include "handler/VkPhysicalDeviceHandler.h"
#include "application/Window.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/Shaders/ShaderLibrary.h"
#include "rendering/SwapChain.h"
#include <chrono>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <memory>
#include <vector>

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>

#include "application/DebugLayer.h"

#include "rendering/GPUImage.h"

class Application
{

    public:
    void Run();
    void UpdateWindowSize(int width, int height);

    Application();
    ~Application();

    private:

    // RAII is first in last out order
    Window _window;
    VkInstanceHandler _instance;
    DebugLayer _vkDebugLayer;
    VkSurfaceHandler _windowSurface;
    // Implicitly linked to instance, see wtd later
    VkPhysicalDeviceHandler _physicalDevice;
    VkDeviceHandler _device;
    //Implicitly linked to device, see wtd later
    VkQueueHandler _graphicsComputeQueue;
    VkQueueHandler _presentQueue;

    // Dynamic lifetime
    std::unique_ptr<SwapChain> _swapChain;

    // used for object sharing data between cpu and gpu -> due to race condition we need one copy for each frame

    DescriptorAllocator _descriptorAllocator;

    std::array<std::unique_ptr<Registry>, 2> _registries;
    enum class RegistryType
    {
        Mesh = 0,
        Material = 1
    };
    ShaderLibrary _shaderLibrary;

    //GraphicsBuffer _visibleDrawIndirectBuffer;
    //GraphicsBuffer _visibleObjectDataBuffer;
    //VkPipelineLayoutHandler _cullingLayout;

    std::unique_ptr<GPUAllocatedImage> _colorBackBuffer;
    std::unique_ptr<GPUAllocatedImage> _depthBuffer;

    VkCommandPoolHandler _graphicsCommandPool;

    // Datas

    VkSamplerHandler _sampler;

    GraphicsBuffer _cameraDataBuffer;

    MultiFrameObject<CommandBuffer> _commandBuffer;

    // GPU Sync
    MultiFrameHandler<VkSemaphoreHandler> _availableImageSemaphore;
    MultiFrameHandler<VkSemaphoreHandler> _renderFinishedSemaphores;
    MultiFrameHandler<VkFenceHandler>  _waitFence;

    UserInterface _UserInterface;
    CullingModule _cullingModule;
    DrawModule _opaqueDrawModule;

    bool _mouseLocked = true;
    float _inputPrevent = 0;
    double _lastMouseX = -1.0;
    double _lastMouseY = -1.0;
    uint32_t _inputDeltaPrevent = 3; // see : https://github.com/glfw/glfw/issues/2523
    bool _wireframe = false;
    static void MouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
    static void WindowFocusCallback(GLFWwindow* window, int focused);
    static void ResizeCallback(GLFWwindow* window, int width, int height);

    Camera _camera;

    std::chrono::time_point<std::chrono::system_clock> _lastFrameTime = std::chrono::system_clock::now();

    void RefreshSwapChain();

    void UpdateCameraData();
    void RecordCommandBuffer(const CommandBuffer& cmdBuffer, uint32_t imageIndex);
    void DrawFrame();
    uint32_t LoadDefaultMaterial();

    // Utils
    static inline const std::vector<const char*> _deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME
    };

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    //Actual logic
    void MainLoop();
};
