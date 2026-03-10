#include "header/VkSurfaceHandler.h"
#include "header/VkDebugLayer.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

VkSurfaceHandler::VkSurfaceHandler(VkInstance instance, GLFWwindow* pWindow)
{
    VkDebugLayer::Log(VkDebugLayer::LogType::CREATE, "VkSurfaceHandler creation !");
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
    VkDebugLayer::Log(VkDebugLayer::LogType::DESTROY, "VkSurfaceHandler Destroyed !");
}
