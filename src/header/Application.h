#pragma once


#include "header/Camera.h"
#include "header/MeshRegistry.h"
#include "header/Renderer.h"
#include "header/VkHandlers/VkSamplerHandler.h"
#include "header/CommandBuffer.h"
#include "header/MultiFrame.h"
#include "header/VkHandlers/VkCommandPoolHandler.h"
#include "header/VkHandlers/VkDescriptorPoolHandler.h"
#include "header/VkHandlers/VkDescriptorSetLayoutHandler.h"
#include "header/VkHandlers/VkDeviceHandler.h"
#include "header/VkHandlers/VkFenceHandler.h"
#include "header/VkHandlers/VkInstanceHandler.h"
#include "header/VkHandlers/VkPhysicalDeviceHandler.h"
#include "header/VkHandlers/VkPipelineHandler.h"
#include "header/VkHandlers/VkPipelineLayoutHandler.h"
#include "header/VkHandlers/VkQueueHandler.h"
#include "header/VkHandlers/VkSemaphoreHandler.h"
#include "header/VkHandlers/VkSurfaceHandler.h"
#include "header/Window.h"
#include "header/GraphicsBuffer.h"
#include "header/SwapChain.h"
#include <chrono>
#include <cstdint>
#include <tiny_obj_loader.h>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <memory>
#include <vector>

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>

#include "DebugLayer.h"

#include "header/GPUImage.h"

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
    VkInstanceHandler _instanceHandler;
    DebugLayer _vkDebugLayer;
    VkSurfaceHandler _windowSurfaceHandler;
    // Implicitly linked to instance, see wtd later
    VkPhysicalDeviceHandler _physicalDeviceHandler;
    VkDeviceHandler _deviceHandler;
    //Implicitly linked to device, see wtd later
    VkQueueHandler _graphicsQueueHandler;
    VkQueueHandler _presentQueueHandler;

    // Dynamic lifetime
    std::unique_ptr<SwapChain> _swapChain;

    // layout binding
    // Layout -> General description
    // Pool -> memory pool to allocate something
    // Set -> Object allocated on the pool
    // So, layout and pool are the memory and need to be cleaned, but as we clean layout, we also clean set automatically

    VkDescriptorSetLayoutHandler _descriptorSetLayoutHandler;

    VkPipelineLayoutHandler _pipelineLayoutHandler;
    VkPipelineHandler _graphicsPipelineHandler;

    std::unique_ptr<GPUAllocatedImage> _colorBackBuffer;
    std::unique_ptr<GPUAllocatedImage> _depthBuffer;

    VkCommandPoolHandler _graphicsCommandPoolHandler;

    // Datas
    MeshRegistry _meshRegistry;
    GPUAllocatedImage _modelImg;
    VkSamplerHandler _sampler;

    VkDescriptorPoolHandler _dynamicDescriptorPoolHandler;
    VkDescriptorPoolHandler _staticDescriptorPoolHandler;

    MultiFrameHandler<GraphicsBuffer> _cameraDataBuffer;

    MultiFrameObject<VkDescriptorSet> _descriptorSet;
    MultiFrameObject<VkDescriptorSet> CreateDescriptorSet();
    MultiFrameObject<CommandBuffer> _commandBuffer;

    // GPU Sync
    MultiFrameHandler<VkSemaphoreHandler> _availableImageSemaphore;
    MultiFrameHandler<VkSemaphoreHandler> _renderFinishedSemaphores;
    MultiFrameHandler<VkFenceHandler>  _waitFence;

    std::vector<Renderer> _renderers;

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

    // Utils
    static inline const std::vector<const char*> _deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    //Actual logic
    void MainLoop();
};
