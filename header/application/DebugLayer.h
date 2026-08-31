#pragma once
#include <string_view>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <vector>

#define M3VK_VERBOSE_LOG 1

class DebugLayer
{
    public:
    #ifdef NDEBUG
    static const bool Enabled = false;
    #else
    static const bool Enabled = true;
    #endif
    inline static const std::vector<const char*> ValidationLayer {
        "VK_LAYER_KHRONOS_validation"
    };

    enum LogType
    {
        VERBOSE,
        INFO,
        WARNING,
        ERROR,
        CREATE,
        DESTROY
    };

    static inline const char* TextColorGrey = "\033[0m";
    static inline const char* TextColorYellow = "\033[33m";
    static inline const char* TextColorRed = "\033[31m";

    DebugLayer();
    ~DebugLayer();

    static void PopulateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo);
    static void SetupCreateInfo(vk::InstanceCreateInfo& instanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT& debugInfoCreate);
    static void Log(LogType LogType, std::string_view message);
    static bool CheckValidationLayerSupport();

    private:
    vk::DebugUtilsMessengerEXT _debugMessenger;

};
