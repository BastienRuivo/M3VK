#ifndef HELLOTRIANGLE_APP_CLASS
#define HELLOTRIANGLE_APP_CLASS

#include "header/SwapChain.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "VkDebugLayer.h"

class HelloTriangleApp
{
    static const int WindowWidth = 960;
    static const int WindowHeight = 540;
    static const int MaxFrameInCount = 2;

    public:
    void Run();
    void FramebufferResized();

    private:
    struct Vertex
    {
        glm::vec2 pos;
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
            attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
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
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };

    const std::vector<int> _indices = {
        0, 1, 2, 2, 3, 0
    };

    bool _framebufferResized;
    uint32_t _currentFrame = 0;
    GLFWwindow* _pWindow = nullptr;
    VkInstance _instance;
    VkDevice _device;
    VkPhysicalDevice _physicalDevice;
    VkDebugLayer _vkDebugLayer;
    // Window API surface
    VkSurfaceKHR _windowSurface;
    std::vector<VkFramebuffer> _framebuffers;
    VkCommandPool _graphicsCommandPool;

    // Queues
    // implicitly cleaned
    VkQueue _graphicsQueue;
    VkQueue _presentQueue;
    VkQueue _copyQueue;
    SwapChain _swapChain;
    std::vector<VkCommandBuffer> _commandBuffers;

    // Uniform container
    VkRenderPass _renderPass;
    VkPipelineLayout _pipelineLayout;
    VkPipeline _graphicsPipeline;

    // GPU Sync
    std::vector<VkSemaphore> _availableImageSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence>  _waitFences;
    // CPU Sync
    int ScoreDeviceSuitability(const VkPhysicalDevice& device) const;

    // Datas
    VkBuffer _vertexBuffer;
    VkDeviceMemory _vertexBufferMemory;
    VkBuffer _indexBuffer;
    VkDeviceMemory _indexBufferMemory;

    // Inits
    void InitWindow();
    void InitVulkan();
    void CreateVKInstance();
    void CreateWindowSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateGraphicsPipeline();
    void CreateRenderPass();
    void CreateFrameBuffers();
    void CreatCommandPool();
    void CreateIndexBuffer();
    void CreateVertexBuffer();
    void CreateCommandBuffers();
    void CreateSyncObject();

    void DisposeSwapChain();
    void RefreshSwapChain();

    void RecordCommandBuffer(VkCommandBuffer cmdBuffer, uint32_t imageIndex);
    void DrawFrame();

    // Utils
    std::vector<const char*> GetRequiredExtensions() const;
    const std::vector<const char*> _deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    bool CheckDeviceExtensionSupport(const VkPhysicalDevice& device) const;

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;


    //Actual logic
    void MainLoop();

    // Disposal
    void Dispose();
    void DisposeWindow();
};

#endif
