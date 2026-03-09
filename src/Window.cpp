#include "header/Window.h"
#include "header/VkDebugLayer.h"
#include <stdexcept>

Window::Window(int width, int height, const char* title, void* resizeObject, GLFWframebuffersizefun resizeCallback)
{
    VkDebugLayer::Log(VkDebugLayer::LogType::CREATE, "Window Creation !");

    _width = width;
    _height = height;
    _title = title;

    glfwInit();
    // GLFW is made for GL (No shit) so create need an empty API for vk
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    _pWindow = glfwCreateWindow(_width, _height, _title, nullptr, nullptr);
    glfwSetWindowUserPointer(_pWindow, resizeObject);
    glfwSetFramebufferSizeCallback(_pWindow, resizeCallback);
}

void Window::CreateWindowSurface(const VkInstance & _instance, VkSurfaceKHR& surface) const
{
    if(glfwCreateWindowSurface(_instance, _pWindow, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create windows surface !");
    }
}

void Window::GetFramebufferSize(int& width, int& height) const
{
    glfwGetFramebufferSize(_pWindow, &width, &height);
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(_pWindow);
}

void Window::ProcessEvent() const
{
    glfwPollEvents();
}

void Window::ResizeWindow(int width, int height)
{
    _width = width;
    _height = height;
}

Window::~Window()
{
    glfwDestroyWindow(_pWindow);
    glfwTerminate();

    VkDebugLayer::Log(VkDebugLayer::LogType::DESTROY, "Window Destroyed !");
}
