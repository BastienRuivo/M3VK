#include "header/VkHandlers/VkSurfaceHandler.h"
#include "header/DebugLayer.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

VkSurfaceHandler::VkSurfaceHandler(VkInstance instance, GLFWwindow* pWindow)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkSurfaceHandler creation !");
#endif

    _instance = instance;
    if(glfwCreateWindowSurface(_instance, pWindow, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create windows surface !");
    }
}

VkSurfaceHandler::~VkSurfaceHandler()
{
    if(_internal == VK_NULL_HANDLE) return;
    vkDestroySurfaceKHR(_instance, _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkSurfaceHandler Destroyed !");
#endif
}

VkSurfaceHandler::VkSurfaceHandler(VkSurfaceHandler && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkSurfaceHandler Move Creation !");
#endif

    _internal = other._internal;
    _instance = other._instance;
    other._internal = VK_NULL_HANDLE;
}

VkSurfaceHandler& VkSurfaceHandler::operator=(VkSurfaceHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        _instance = other._instance;
        other._internal = VK_NULL_HANDLE;
    }

    return *this;
}
