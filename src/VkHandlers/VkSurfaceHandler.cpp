#include "header/VkHandlers/VkSurfaceHandler.h"
#include "header/DebugLayer.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

VkSurfaceHandler::VkSurfaceHandler(VkInstance instance, GLFWwindow* pWindow)
{
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkSurfaceHandler creation !");
    _instance = instance;
    if(glfwCreateWindowSurface(_instance, pWindow, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create windows surface !");
    }
}

VkSurfaceKHR VkSurfaceHandler::Get() const
{
    return _internal;
}

VkSurfaceHandler::~VkSurfaceHandler()
{
    vkDestroySurfaceKHR(_instance, _internal, nullptr);
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkSurfaceHandler Destroyed !");
}
