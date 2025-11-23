#ifndef HELLOTRIANGLE_APP_CLASS
#define HELLOTRIANGLE_APP_CLASS

#include "header/SwapChain.h"
#include <cstdint>
#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VkDebugLayer.h"

class HelloTriangleApp
{
    static const int WindowWidth = 960;
    static const int WindowHeight = 540;

    public:
    void Run();

    private:
    GLFWwindow* _pWindow = nullptr;
    VkInstance _instance;
    VkDevice _device;
    VkPhysicalDevice _physicalDevice;
    VkDebugLayer _vkDebugLayer;
    // Window API surface
    VkSurfaceKHR _windowSurface;
    std::vector<VkFramebuffer> _framebuffers;
    VkCommandPool _commandPool;


    // Queues
    // implicitly cleaned
    VkQueue _graphicsQueue;
    VkQueue _presentQueue;
    SwapChain _swapChain;
    VkCommandBuffer _commandBuffer;

    // Uniform container
    VkRenderPass _renderPass;
    VkPipelineLayout _pipelineLayout;
    VkPipeline _graphicsPipeline;

    // GPU Sync
    VkSemaphore _availableImageSemaphore;
    VkSemaphore _renderFinishedSemaphore;
    VkFence _waitFence;
    // CPU Sync

    int ScoreDeviceSuitability(const VkPhysicalDevice& device) const;

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
    void CreateCommandBuffer();
    void CreateSyncObject();

    void RecordCommandBuffer(VkCommandBuffer cmdBuffer, uint32_t imageIndex);
    void DrawFrame();

    // Utils
    std::vector<const char*> GetRequiredExtensions() const;
    const std::vector<const char*> _deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    bool CheckDeviceExtensionSupport(const VkPhysicalDevice& device) const;


    //Actual logic
    void MainLoop();

    // Disposal
    void Dispose();
    void DisposeWindow();
};

#endif
