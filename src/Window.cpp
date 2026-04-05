#include "header/Window.h"

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

void Window::SetIcon(unsigned char* pixels, int width, int height)
{
    GLFWimage image;
    image.pixels = pixels;
    image.width = width;
    image.height = height;
    glfwSetWindowIcon(_internal, 1, &image);
}

Window::Window(int width, int height, const char* title, void* resizeObject, GLFWframebuffersizefun resizeCallback)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "Window Creation !");
#endif

    _width = width;
    _height = height;
    _title = title;

    glfwInit();
    // GLFW is made for GL (No shit) so create need an empty API for vk
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    _internal = glfwCreateWindow(_width, _height, _title, nullptr, nullptr);
    glfwSetWindowUserPointer(_internal, resizeObject);
    glfwSetFramebufferSizeCallback(_internal, resizeCallback);
}

void Window::GetFramebufferSize(int& width, int& height) const
{
    glfwGetFramebufferSize(_internal, &width, &height);
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(_internal);
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
    glfwDestroyWindow(_internal);
    glfwTerminate();

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "Window Destroyed !");
#endif
}
