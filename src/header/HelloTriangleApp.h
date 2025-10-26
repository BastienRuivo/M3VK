#ifndef HELLOTRIANGLE_APP_CLASS
#define HELLOTRIANGLE_APP_CLASS

#include <cstdint>
#include <optional>
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

    struct QueueFamilyId
    {
        std::optional<uint32_t> Graphics;
        std::optional<uint32_t> Present;

        static bool AreAllQueueAvailable(const QueueFamilyId& queueIds)
        {
            return queueIds.Graphics.has_value()
                && queueIds.Present.has_value();
        }
    };

    public:
    void Run();

    private:
    GLFWwindow* _pWindow = nullptr;
    VkInstance _instance;
    VkDevice _logicalDevice;
    VkPhysicalDevice _physicalDevice;
    VkDebugLayer _vkDebugLayer;
    // Window API surface
    VkSurfaceKHR _windowSurface;

    // Queues
    // implicitly cleaned
    VkQueue _graphicsQueue;
    VkQueue _presentQueue;

    int ScoreDeviceSuitability(const VkPhysicalDevice& device) const;

    // Inits
    void InitWindow();
    void InitVulkan();
    void CreateVKInstance();
    void CreateWindowSurface();

    // Logical Device
    void CreateLogicalDevice();
    // Physical Device
    void PickPhysicalDevice();

    // Queue Utility
    QueueFamilyId FindQueueFamilies(const VkPhysicalDevice& device) const;

    // Utils
    std::vector<const char*> GetRequiredExtensions() const;

    //Actual logic
    void MainLoop();

    // Disposal
    void Dispose();
    void DisposeWindow();
};

#endif
