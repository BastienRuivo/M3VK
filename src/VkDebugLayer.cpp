#include "./header/VkDebugLayer.h"
#include <ostream>
#include <vulkan/vulkan_core.h>
#include <iostream>
#include <cstring>

VkResult M3VK_CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void M3VK_DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void VkDebugLayer::Log(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, const std::string& message)
{
    VkDebugLayer::LogType logType;
    switch (messageSeverity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: logType = VkDebugLayer::VERBOSE; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: logType = VkDebugLayer::INFO; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: logType = VkDebugLayer::WARNING; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: logType = VkDebugLayer::ERROR; break;
        default: logType = VkDebugLayer::VERBOSE; break;
    }
    Log(logType, message);
}

void VkDebugLayer::Log(VkDebugLayer::LogType LogType, const std::string& message)
{
    if(!Enabled) return;
    #ifndef M3VK_VERBOSE_LOG
        if(LogType == VkDebugLayer::LogType::INFO || VkDebugLayer::LogType::VERBOSE) return;
    #endif

    #ifndef M3VK_MEMORYLOG
        if(LogType == VkDebugLayer::LogType::CREATE || VkDebugLayer::LogType::DESTROY) return;
    #endif


    const char * color = nullptr;
    const char * title = nullptr;
    std::ostream * stream = nullptr;

    // Stream selection
    switch (LogType)
    {
        case VkDebugLayer::LogType::WARNING:
        case VkDebugLayer::LogType::ERROR: stream = &std::cerr; break;
        default: stream = &std::clog; break;
    }

    // Color selection
    switch (LogType)
    {
        case VkDebugLayer::LogType::WARNING: color = TextColorYellow; break;
        case VkDebugLayer::LogType::ERROR: color = TextColorRed; break;
        default: color = TextColorGrey; break;
    }

    // Title selection

    switch (LogType)
    {
        case VkDebugLayer::LogType::WARNING: title = "Warning"; break;
        case VkDebugLayer::LogType::ERROR: title = "Error"; break;
        case VkDebugLayer::LogType::VERBOSE: title = "Verbose"; break;
        case VkDebugLayer::LogType::INFO: title = "Info"; break;
        case VkDebugLayer::LogType::CREATE: title = "Create"; break;
        case VkDebugLayer::LogType::DESTROY: title = "Destroy"; break;
        default: title = "Unknown"; break;
    }

    *stream << color << "[Validation Layer Message] - [M3VK] : [" << title << "] " << message << std::endl;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{

    #ifndef M3VK_VERBOSE_LOG
        if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) return true;
    #endif

    std::cerr << "[Validation Layer Message] - [Vulkan] : ";
    switch (messageSeverity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            std::cerr << "\033[0m" << "[Verbose]";
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            std::cerr << "\033[0m" << "[Info]";
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            std::cerr << "\033[33m" << "[Warning]";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            std::cerr << "\033[31m" << "[Error]";
            break;
        default:
            break;
    }

    switch (messageType)
    {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            std::cerr<<"[General]";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            std::cerr<<"[Validation]";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            std::cerr<<"[Performance]";
            break;
    }

    std::cerr << pCallbackData->pMessage << std::endl;

    return messageSeverity != VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
}

bool VkDebugLayer::CheckValidationLayerSupport()
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for(const char* layerName : validationLayer)
    {
        bool find = false;
        for(const VkLayerProperties& property : availableLayers)
        {
            if(strcmp(layerName, property.layerName) == 0)
            {
                find = true;
                break;
            }
        }

        if(!find)
        {
            std::cerr<<"Can't find validation layer "<<layerName<<std::endl;
            return false;
        }
    }

    return true;
}

void VkDebugLayer::SetupCreateInfo(VkInstanceCreateInfo& instanceCreateInfo, VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo)
{
    if(Enabled)
    {
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayer.size());
        instanceCreateInfo.ppEnabledLayerNames = validationLayer.data();
        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        instanceCreateInfo.pNext = &debugCreateInfo;
    }
    else
    {
        instanceCreateInfo.enabledLayerCount = 0;
        instanceCreateInfo.pNext = nullptr;
    }
}

void VkDebugLayer::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugMessengerCreateInfo)
{
    debugMessengerCreateInfo = {};
    debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugMessengerCreateInfo.pfnUserCallback = DebugCallback;
}

void VkDebugLayer::Create(VkInstance instance)
{
    if(!Enabled) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    PopulateDebugMessengerCreateInfo(createInfo);

    if (M3VK_CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &_debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void VkDebugLayer::Dispose(VkInstance instance)
{
    if (!Enabled) return;
    M3VK_DestroyDebugUtilsMessengerEXT(instance, _debugMessenger, nullptr);
}
