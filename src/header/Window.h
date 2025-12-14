#ifndef WINDOW_CLASS
#define WINDOW_CLASS


#include <stdexcept>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window
{
    public:
    void init(int width, int height, const char* title, void* resizeObject, GLFWframebuffersizefun resizeCallback)
    {
        _width = width;
        _height = height;
        _title = title;

        glfwInit();
        // GLFW is made for GL (No shit) so create need an empty API for vk
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        _pWindow = glfwCreateWindow(_width, Window::_height, _title, nullptr, nullptr);
        glfwSetWindowUserPointer(_pWindow, resizeObject);
        glfwSetFramebufferSizeCallback(_pWindow, resizeCallback);
    }

    void CreateWindowSurface(const VkInstance & _instance, VkSurfaceKHR& surface) const
    {
        if(glfwCreateWindowSurface(_instance, _pWindow, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create windows surface !");
        }
    }

    void GetFramebufferSize(int& width, int& height) const
    {
        glfwGetFramebufferSize(_pWindow, &width, &height);
    }

    bool ShouldClose() const
    {
        return glfwWindowShouldClose(_pWindow);
    }

    void ProcessEvent() const
    {
        glfwPollEvents();
    }

    void Dispose()
    {

    }


    private:
    GLFWwindow* _pWindow = nullptr;
    int _width;
    int _height;
    const char * _title;
};

#endif
