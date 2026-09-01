#include <vulkan/vulkan.hpp>

#include "application/DebugLayer.h"
#include "application/ApplicationInfo.h"
#include <ostream>
#include <string_view>
#include <iostream>
#include <cstring>


void DebugLayer::Log(DebugLayer::LogType LogType, std::string_view message)
{
    if(!Enabled) return;

    const char * color = nullptr;
    const char * title = nullptr;
    std::ostream * stream = nullptr;

    // Stream selection
    switch (LogType)
    {
        case DebugLayer::LogType::WARNING:
        case DebugLayer::LogType::ERROR: stream = &std::cerr; break;
        default: stream = &std::clog; break;
    }

    // Color selection
    switch (LogType)
    {
        case DebugLayer::LogType::WARNING: color = TextColorYellow; break;
        case DebugLayer::LogType::ERROR: color = TextColorRed; break;
        default: color = TextColorGrey; break;
    }

    // Title selection

    switch (LogType)
    {
        case DebugLayer::LogType::WARNING: title = "Warning"; break;
        case DebugLayer::LogType::ERROR: title = "Error"; break;
        case DebugLayer::LogType::VERBOSE: title = "Verbose"; break;
        case DebugLayer::LogType::INFO: title = "Info"; break;
        case DebugLayer::LogType::CREATE: title = "Create"; break;
        case DebugLayer::LogType::DESTROY: title = "Destroy"; break;
        default: title = "Unknown"; break;
    }

    *stream << color << "[Validation Layer Message] - [M3VK] : [" << title << "] " << message << std::endl;
}

static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{

    #ifndef M3VK_VERBOSE_LOG
        if(messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose) return true;
    #endif

    std::cerr << "[Validation Layer Message] - [Vulkan] : ";
    switch (messageSeverity)
    {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
            std::cerr << "\033[0m" << "[Verbose]";
            break;

        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
            std::cerr << "\033[0m" << "[Info]";
            break;

        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            std::cerr << "\033[33m" << "[Warning]";
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            std::cerr << "\033[31m" << "[Error]";
            break;
        default:
            break;
    }

    if (messageType & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
    {
        std::cerr<<"[General]";
    }
    if (messageType & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
    {
        std::cerr<<"[Validation]";
    }
    if (messageType & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
    {
        std::cerr<<"[Performance]";
    }

    std::cerr << pCallbackData->pMessage << std::endl;

    return messageSeverity != vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
}

bool DebugLayer::CheckValidationLayerSupport()
{
    std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    for(const char* layerName : ValidationLayer)
    {
        bool find = false;
        for(const vk::LayerProperties& property : availableLayers)
        {
            if(strcmp(layerName, property.layerName.data()) == 0)
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

void DebugLayer::SetupCreateInfo(vk::InstanceCreateInfo& instanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT& debugCreateInfo)
{
    if(Enabled)
    {
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayer.size());
        instanceCreateInfo.ppEnabledLayerNames = ValidationLayer.data();
        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        instanceCreateInfo.pNext = &debugCreateInfo;
    }
    else
    {
        instanceCreateInfo.enabledLayerCount = 0;
        instanceCreateInfo.pNext = nullptr;
    }
}

void DebugLayer::PopulateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& debugMessengerCreateInfo)
{
    debugMessengerCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT{}
        .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
        .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
        .setPfnUserCallback(static_cast<vk::PFN_DebugUtilsMessengerCallbackEXT>(DebugCallback));
}

DebugLayer::DebugLayer(const vk::raii::Instance& instance)
{
    if(!Enabled) return;

    vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
    PopulateDebugMessengerCreateInfo(createInfo);

    vk::Result result = vk::Instance(instance).createDebugUtilsMessengerEXT(&createInfo, nullptr, &_debugMessenger);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

DebugLayer::~DebugLayer()
{
    if (!Enabled) return;
    ApplicationInfo::Instance().destroyDebugUtilsMessengerEXT(_debugMessenger);
}
