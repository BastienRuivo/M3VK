#pragma once

#include "Material.h"
#include "application/Pipeline.h"
#include "application/UserInterface.h"
#include "modules/DrawModule.h"
#include "rendering/CommandBuffer.h"
#include "rendering/MultiFrame.h"
#include "allocation/RaiiHelper.h"
#include "application/Window.h"
#include "rendering/SwapChain.h"
#include <array>
#include <chrono>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <memory>
#include <vector>

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include "application/DebugLayer.h"

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
    vk::raii::Context _context;
    vk::raii::Instance _instance;
    DebugLayer _vkDebugLayer;
    vk::raii::SurfaceKHR _windowSurface;
    vk::raii::PhysicalDevice _physicalDevice;
    vk::raii::Device _device;

    ApplicationInfo::Initializer _appInfoInitializer;

    vk::raii::Queue _graphicsComputeQueue;
    vk::raii::Queue _presentQueue;

    // Dynamic lifetime
    vk::raii::CommandPool _graphicsCommandPool;
    std::unique_ptr<SwapChain> _swapChain;

    // used for object sharing data between cpu and gpu -> due to race condition we need one copy for each frame

    UserInterface _userInterface;

    Pipeline _pipeline;

    MultiFrameObject<CommandBuffer> _commandBuffer;

    // GPU Sync
    MultiFrameObject<vk::raii::Semaphore> _availableImageSemaphore;
    MultiFrameObject<vk::raii::Semaphore> _renderFinishedSemaphores;
    MultiFrameObject<vk::raii::Fence>  _waitFence;

    bool _mouseLocked = true;
    float _inputPrevent = 0;
    double _lastMouseX = -1.0;
    double _lastMouseY = -1.0;
    uint32_t _inputDeltaPrevent = 3; // see : https://github.com/glfw/glfw/issues/2523

    static void MouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
    static void WindowFocusCallback(GLFWwindow* window, int focused);
    static void ResizeCallback(GLFWwindow* window, int width, int height);


    std::chrono::time_point<std::chrono::system_clock> _lastFrameTime = std::chrono::system_clock::now();

    void RefreshSwapChain();
    void DrawFrame();
    std::array<DrawModule, MaterialType::Count> InitDrawModule(bool DebugOn);

    // Utils
    static inline const std::vector<const char*> _deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME
    };

    //Actual logic
    void MainLoop();
};
