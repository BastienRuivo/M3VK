#ifndef HELLOTRIANGLE_APP_CLASS
#define HELLOTRIANGLE_APP_CLASS

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
    VkDebugLayer _vkDebugLayer;

    // Inits
    void InitWindow();
    void InitVulkan();
    void CreateVKInstance();

    // Utils
    std::vector<const char*> GetRequiredExtensions() const;

    //Actual logic
    void MainLoop();

    // Disposal
    void Dispose();
    void DisposeWindow();
};

#endif
