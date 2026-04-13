#include "header/VkHandlers/VkInstanceHandler.h"
#include "header/DebugLayer.h"
#include <cstring>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

VkInstanceHandler::VkInstanceHandler()
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkInstanceHandler creation !");
#endif

    VkApplicationInfo appInfo
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "M3VK",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_4
    };

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, supportedExtensions.data());

    if(DebugLayer::Enabled)
    {
#ifdef M3VK_VERBOSE_LOG
        DebugLayer::Log(DebugLayer::LogType::INFO, "List of actives VK Extensions");
#endif
        for(const VkExtensionProperties& extension : supportedExtensions)
        {
#ifdef M3VK_VERBOSE_LOG
            DebugLayer::Log(DebugLayer::LogType::INFO, std::string("\t - ") + extension.extensionName);
#endif
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

    if(!DebugLayer::CheckValidationLayerSupport())
    {
        throw std::runtime_error("Validation layer requested but not available !");
    }

    VkInstanceCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data(),
    };

    // ensure it is not destroyed before vkCreateInstance
    VkDebugUtilsMessengerCreateInfoEXT debugInfoCreate;
    DebugLayer::SetupCreateInfo(createInfo, debugInfoCreate);

    VkResult result = vkCreateInstance(&createInfo, nullptr, &_internal);
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

std::vector<const char *> VkInstanceHandler::GetRequiredExtensions() const
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if(DebugLayer::Enabled)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}
VkInstanceHandler::~VkInstanceHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroyInstance(_internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkInstanceHandler Destroyed !");
#endif
}

VkInstanceHandler::VkInstanceHandler(VkInstanceHandler && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkInstanceHandler Move Creation !");
#endif

    _internal = other._internal;
    other._internal = VK_NULL_HANDLE;
}

VkInstanceHandler& VkInstanceHandler::operator=(VkInstanceHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        other._internal = VK_NULL_HANDLE;
    }

    return *this;
}
