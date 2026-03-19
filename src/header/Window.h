#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window
{
    public:
    Window(int width, int height, const char* title, void* resizeObject, GLFWframebuffersizefun resizeCallback);
    ~Window();
    void ResizeWindow(int width, int height);

    bool ShouldClose() const;
    void ProcessEvent() const;
    void GetFramebufferSize(int& width, int& height) const;

    inline GLFWwindow* Get() const
    {
        return _internal;
    }


    private:
    GLFWwindow* _internal = nullptr;
    int _width;
    int _height;
    const char * _title;
};
