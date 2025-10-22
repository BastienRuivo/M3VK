#include "header/HelloTriangleApp.h"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>



void HelloTriangleApp::InitWindow()
{
    glfwInit();
    // GLFW is made for GL (No shit) so create need an empty API for vk
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // TODO : Handle RESIZABLE window later
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _pWindow = glfwCreateWindow(HelloTriangleApp::WindowWidth, HelloTriangleApp::WindowHeight, "M3VK", nullptr, nullptr);
}

void HelloTriangleApp::InitVulkan()
{
    CreateVKInstance();
    _vkDebugLayer.Create(_instance);
}

void HelloTriangleApp::CreateVKInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "M3VK";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_4;

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, supportedExtensions.data());

#ifdef M3VK_VERBOSE_LOG
    std::cout << "List of actives VK extensions" << std::endl;
    for(const VkExtensionProperties& extension : supportedExtensions)
    {
        std::cout << "\t - " << extension.extensionName << std::endl;
    }
#endif

    std::vector<const char *> requiredExtensions = GetRequiredExtensions();
    for(int i = 0; i < requiredExtensions.size(); ++i)
    {
        bool isPresent = false;
        for(const VkExtensionProperties& extension : supportedExtensions)
        {
            if(strcmp(requiredExtensions[i], extension.extensionName) == 0)
            {
                isPresent = true;
                break;
            }
        }

        if(!isPresent)
        {
            throw std::runtime_error("Vulkan Extension" + std::string(requiredExtensions[i]) + " Is not supported but required for GLFW");
        }
    }

    if(!_vkDebugLayer.CheckValidationLayerSupport())
    {
        throw std::runtime_error("Validation layer requested but not available !");
    }

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    // ensure it is not destroyed before vkCreateInstance
    VkDebugUtilsMessengerCreateInfoEXT debugInfoCreate;
    _vkDebugLayer.SetupCreateInfo(createInfo, debugInfoCreate);

    VkResult result = vkCreateInstance(&createInfo, nullptr, &_instance);
    if(result != VK_SUCCESS)
    {
        if(result == VK_ERROR_LAYER_NOT_PRESENT)
        {
            throw std::runtime_error("Error : A VK Layer is not present on computer");
        }
        else
        {
            throw std::runtime_error("Failed to create VK_Instance");
        }
    }
}

std::vector<const char *> HelloTriangleApp::GetRequiredExtensions() const
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if(_vkDebugLayer.Enabled)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void HelloTriangleApp::MainLoop()
{
    while (!glfwWindowShouldClose(_pWindow))
    {
        glfwPollEvents();
    }
}

void HelloTriangleApp::DisposeWindow()
{
    glfwDestroyWindow(_pWindow);
    glfwTerminate();
}

void HelloTriangleApp::Dispose()
{
    _vkDebugLayer.Dispose(_instance);
    vkDestroyInstance(_instance, nullptr);
    DisposeWindow();
}

void HelloTriangleApp::Run()
{
    InitWindow();
    InitVulkan();
    MainLoop();
    Dispose();
}
