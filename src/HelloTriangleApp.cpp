#include "header/HelloTriangleApp.h"
#include "header/M3VKHelper.h"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <cstring>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

bool HelloTriangleApp::CheckDeviceExtensionSupport(const VkPhysicalDevice& device) const
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> properties(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, properties.data());

    for(const auto& extension : _deviceExtensions)
    {
        bool foundExtension = false;
        for(const VkExtensionProperties& property : properties)
        {
            if(strcmp(extension, property.extensionName) == 0)
            {
                foundExtension = true;
                break;
            }
        }
        if(!foundExtension)
        {
            if(_vkDebugLayer.Enabled)
            {
                _vkDebugLayer.LogError((std::string("Extension not supported : ") + std::string(extension)).c_str());
            }
            return false;
        }
    }

    return true;
}

void HelloTriangleApp::CreateWindowSurface()
{
    if(glfwCreateWindowSurface(_instance, _pWindow, nullptr, &_windowSurface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create windows surface !");
    }
}

void HelloTriangleApp::CreateLogicalDevice()
{
    M3VKHelper::QueueFamilyId queueFamilyId = M3VKHelper::QueryQueueFamilies(_physicalDevice, _windowSurface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueIds =
    {
        queueFamilyId.Graphics.value(),
        queueFamilyId.Present.value()
    };

    float queuePriority = 1.0f;
    for(uint32_t queueId : uniqueQueueIds)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueId;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(_deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = _deviceExtensions.data();

    if(_vkDebugLayer.Enabled)
    {
        deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(_vkDebugLayer.validationLayer.size());
        deviceCreateInfo.ppEnabledLayerNames = _vkDebugLayer.validationLayer.data();
    }
    else
    {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    VkResult deviceCreation = vkCreateDevice(_physicalDevice, &deviceCreateInfo, nullptr, &_logicalDevice);
    if(deviceCreation != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create VK Logical Device !");
    }

    vkGetDeviceQueue(_logicalDevice, queueFamilyId.Graphics.value(), 0, &_graphicsQueue);
    vkGetDeviceQueue(_logicalDevice, queueFamilyId.Present.value(), 0, &_presentQueue);
}

int HelloTriangleApp::ScoreDeviceSuitability(const VkPhysicalDevice& device) const
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // support of addtionnal feature (texture compression, 64bit double, multi viewport rendering)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int score = 0;


    M3VKHelper::QueueFamilyId ids = M3VKHelper::QueryQueueFamilies(device, _windowSurface);

    bool areAllRequiredExtensionsSupported = CheckDeviceExtensionSupport(device);

    M3VKHelper::SwapChainSupportDetails swapChainDetails = M3VKHelper::QuerySwapChainSupportDetail(device, _windowSurface);

    // Mandatory feature, if any return 0 and this will be the only way to have 0 score meaning there's no suitable GPU
    if(!M3VKHelper::QueueFamilyId::AreAllQueueAvailable(ids)
        || !areAllRequiredExtensionsSupported
        || !swapChainDetails.CheckSwapChainSupportAdequate())
    {
        return 0;
    }

    // Else we try to find the best available GPU for our criteria
    switch (deviceProperties.deviceType)
    {
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: score += 600; break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: score += 800; break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: score += 1000; break;
        default: break;
    }

    return score;
}

void HelloTriangleApp::PickPhysicalDevice()
{
    _physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

    if(deviceCount == 0)
    {
        throw std::runtime_error("Failed to find a Vulkan compatible GPU on this device");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

    int bestScore = 0;
    for(const VkPhysicalDevice& device : devices)
    {
        int score = ScoreDeviceSuitability(device);
        if(score > bestScore)
        {
            bestScore = score;
            _physicalDevice = device;
        }
    }

    if(_physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable GPU on this device");
    }
}

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
    CreateWindowSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    _swapChain.Create(_pWindow, _physicalDevice, _logicalDevice, _windowSurface);
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

    if(_vkDebugLayer.Enabled)
    {
        _vkDebugLayer.LogInfo("List of actives VK Extensions");
        for(const VkExtensionProperties& extension : supportedExtensions)
        {
            _vkDebugLayer.LogInfo(std::string("\t - ") + extension.extensionName);
        }
    }


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
    _swapChain.Dipose(_logicalDevice);
    vkDestroyDevice(_logicalDevice, nullptr);
    _vkDebugLayer.Dispose(_instance);
    vkDestroySurfaceKHR(_instance, _windowSurface, nullptr);
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
