#pragma once

#include "header/CommandBuffer.h"
#include "header/VkHandlers/VkDeviceHandler.h"
#include "header/VkHandlers/VkInstanceHandler.h"
#include "header/VkHandlers/VkPhysicalDeviceHandler.h"
#include "header/VkHandlers/VkQueueHandler.h"
#include "header/VkHandlers/VkRenderPassHandler.h"
#include "header/VkHandlers/VkSurfaceHandler.h"
#include "header/Window.h"
#include "header/GraphicsBuffer.h"
#include "header/SwapChain.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <memory>
#include <vector>

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>

#include "VkDebugLayer.h"

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
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;

        static VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription description{};
            description.binding = 0;
            description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            description.stride = sizeof(Vertex);
            return description;
        }

        static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescription()
        {
            std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

            // position
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            // color
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            return attributeDescriptions;
        }
    };

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
    VkDebugLayer _vkDebugLayer;
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


    // Window API surface
    std::vector<VkFramebuffer> _framebuffers;
    VkCommandPool _graphicsCommandPool;


    // Queues
    // implicitly cleaned
    //VkQueue _copyQueue;

    std::vector<CommandBuffer> _commandBuffers;

    // Uniform container
    VkPipeline _graphicsPipeline;

    // GPU Sync
    std::vector<VkSemaphore> _availableImageSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence>  _waitFences;

    // layout binding
    // Layout -> General description
    // Pool -> memory pool to allocate something
    // Set -> Object allocated on the pool
    // So, layout and pool are the memory and need to be cleaned, but as we clean layout, we also clean set automatically

    VkDescriptorSetLayout _descriptorSetLayout;
    VkPipelineLayout _pipelineLayout;
    VkDescriptorPool _descriptorPool;
    std::vector<VkDescriptorSet> _descriptorSets;

    std::vector<GraphicsBuffer> _cameraDataBuffers;

    // Datas
    GraphicsBuffer _vertexBuffer;
    GraphicsBuffer _indexBuffer;

    void CreateGraphicsPipeline();
    void CreateFrameBuffers();
    void CreatCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObject();
    void CreateDescriptorSetLayout();
    void CreateCameraDataBuffers();
    void CreateDescriptorPool();
    void CreateDescriptorSet();

    void DisposeFramebuffers();
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
