#include "header/Window.h"
#include "header/DebugLayer.h"

Window::Window(int width, int height, const char* title, void* resizeObject, GLFWframebuffersizefun resizeCallback)
{
    DebugLayer::Log(DebugLayer::LogType::CREATE, "Window Creation !");

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

GLFWwindow* Window::Get() const
{
    return _pWindow;
}

Window::~Window()
{
    glfwDestroyWindow(_pWindow);
    glfwTerminate();

    DebugLayer::Log(DebugLayer::LogType::DESTROY, "Window Destroyed !");
}
