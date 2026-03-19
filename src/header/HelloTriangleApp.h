#pragma once

#include "header/CommandBuffer.h"
#include "header/MultiFrame.h"
#include "header/Vertex.h"
#include "header/VkHandlers/VkCommandPoolHandler.h"
#include "header/VkHandlers/VkDescriptorPoolHandler.h"
#include "header/VkHandlers/VkDescriptorSetLayoutHandler.h"
#include "header/VkHandlers/VkDeviceHandler.h"
#include "header/VkHandlers/VkFramebufferHandler.h"
#include "header/VkHandlers/VkInstanceHandler.h"
#include "header/VkHandlers/VkPhysicalDeviceHandler.h"
#include "header/VkHandlers/VkPipelineHandler.h"
#include "header/VkHandlers/VkPipelineLayoutHandler.h"
#include "header/VkHandlers/VkQueueHandler.h"
#include "header/VkHandlers/VkRenderPassHandler.h"
#include "header/VkHandlers/VkSemaphoreHandler.h"
#include "header/VkHandlers/VkSurfaceHandler.h"
#include "header/Window.h"
#include "header/GraphicsBuffer.h"
#include "header/SwapChain.h"
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <memory>
#include <vector>

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>

#include "DebugLayer.h"

class HelloTriangleApp
{
    static const int MaxFrameInCount = 2;

    public:
    void Run();
    void FramebufferResized();
    void UpdateWindowSize(int width, int height);

    HelloTriangleApp();
    ~HelloTriangleApp();

    private:

    const std::vector<Vertex> _vertices = {
        {{-0.5f, 0, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };

    struct CameraData
    {
        // temp
        glm::mat4 localToWorldMatrix;
        glm::mat4 worldToCameraMatrix;
        glm::mat4 projectionMatrix;
        glm::mat4 viewProjectionMatrix;
    };

    const std::vector<int> _indices = {
        0, 1, 2, 2, 3, 0
    };

    bool _framebufferResized;
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

    VkRenderPassHandler _renderPassHandler;

    // layout binding
    // Layout -> General description
    // Pool -> memory pool to allocate something
    // Set -> Object allocated on the pool
    // So, layout and pool are the memory and need to be cleaned, but as we clean layout, we also clean set automatically

    VkDescriptorSetLayoutHandler _descriptorSetLayoutHandler;

    VkPipelineLayoutHandler _pipelineLayoutHandler;
    VkPipelineHandler _graphicsPipelineHandler;

    MultiFrameHandler<VkFramebufferHandler> _framebuffer;
    MultiFrameHandler<VkFramebufferHandler> CreateFramebuffer();
    void InitFramebuffer(MultiFrameHandler<VkFramebufferHandler>& framebuffer);

    VkCommandPoolHandler _graphicsCommandPoolHandler;

    // Datas
    GraphicsBuffer _vertexBuffer;
    GraphicsBuffer _indexBuffer;
    VkDescriptorPoolHandler _descriptorPoolHandler;

    MultiFrameHandler<GraphicsBuffer> _cameraDataBuffer;

    MultiFrameObject<VkDescriptorSet> _descriptorSet;
    MultiFrameObject<VkDescriptorSet> CreateDescriptorSet();
    MultiFrameObject<CommandBuffer> _commandBuffer;

    // GPU Sync
    MultiFrameHandler<VkSemaphoreHandler> _availableImageSemaphore;
    MultiFrameHandler<VkSemaphoreHandler> _renderFinishedSemaphores;
    std::vector<VkFence>  _waitFences;

    void CreatCommandPool();
    void CreateSyncObject();
    void CreateDescriptorPool();

    void RefreshSwapChain();

    void UpdateCameraData(uint32_t currentImage);
    void RecordCommandBuffer(CommandBuffer cmdBuffer, uint32_t currentFrame, uint32_t imageIndex);
    void DrawFrame();

    // Utils
    static inline const std::vector<const char*> _deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    //Actual logic
    void MainLoop();
};
