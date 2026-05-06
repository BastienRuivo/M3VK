#pragma once


#include "rendering/Camera.h"
#include "rendering/DescriptorPool.h"
#include "registry/Registry.h"
#include "rendering/CommandBuffer.h"
#include "rendering/MultiFrame.h"
#include "handler/Handlers.h"
#include "handler/VkPhysicalDeviceHandler.h"
#include "application/Window.h"
#include "rendering/GraphicsBuffer.h"
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

    struct CameraData
    {
        // temp
        alignas(16) glm::mat4 worldToCameraMatrix;
        alignas(16) glm::mat4 projectionMatrix;
        alignas(16) glm::mat4 viewProjectionMatrix;
    };

    uint32_t _currentFrame = 0;

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
    DescriptorPool _dynamicDescriptorPool;
    DescriptorPool _staticDescriptorPool;
    DescriptorSetHandle _materialInstancesSet;

    std::array<std::unique_ptr<Registry>, 2> _registries;
    enum class RegistryType
    {
        Mesh = 0,
        Material = 1
    };

    VkPipelineLayoutHandler _pipelineLayout;
    VkPipelineHandler _graphicsPipeline;

    std::unique_ptr<GPUAllocatedImage> _colorBackBuffer;
    std::unique_ptr<GPUAllocatedImage> _depthBuffer;

    VkCommandPoolHandler _graphicsCommandPool;

    // Datas

    VkSamplerHandler _sampler;

    MultiFrameHandler<GraphicsBuffer> _cameraDataBuffer;
    MultiFrameObject<DescriptorSetHandle> _descriptorSet;
    MultiFrameObject<DescriptorSetHandle> CreateDescriptorSet();

    MultiFrameObject<CommandBuffer> _commandBuffer;

    // GPU Sync
    MultiFrameHandler<VkSemaphoreHandler> _availableImageSemaphore;
    MultiFrameHandler<VkSemaphoreHandler> _renderFinishedSemaphores;
    MultiFrameHandler<VkFenceHandler>  _waitFence;

    bool _mouseLocked = true;
    float _inputPrevent = 0;
    double _lastMouseX = -1.0;
    double _lastMouseY = -1.0;
    uint32_t _inputDeltaPrevent = 3; // see : https://github.com/glfw/glfw/issues/2523
    static void MouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
    static void WindowFocusCallback(GLFWwindow* window, int focused);
    static void ResizeCallback(GLFWwindow* window, int width, int height);

    Camera _camera;

    std::chrono::time_point<std::chrono::system_clock> _lastFrameTime = std::chrono::system_clock::now();

    void RefreshSwapChain();

    void UpdateCameraData(uint32_t currentImage);
    void RecordCommandBuffer(const CommandBuffer& cmdBuffer, uint32_t currentFrame, uint32_t imageIndex);
    void DrawFrame();
    uint32_t LoadDefaultMaterial();

    // Utils
    static inline const std::vector<const char*> _deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    //Actual logic
    void MainLoop();
};
