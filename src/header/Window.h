#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window
{
    public:
    Window(int width, int height, const char* title, void* resizeObject, GLFWframebuffersizefun resizeCallback, GLFWcursorposfun mouseCallback, GLFWwindowfocusfun focusCallback);
    ~Window();
    void ResizeWindow(int width, int height);
    void SetIcon(unsigned char* pixels, int width, int height);

    bool ShouldClose() const;
    void ProcessEvent() const;
    void GetFramebufferSize(int& width, int& height) const;

    inline void QueryMousePose(double& width, double& height) const
    {
        glfwGetCursorPos(_internal, &width, &height);
    }

    inline bool IsKeyPressed(int key) const
    {
        return glfwGetKey(_internal, key) == GLFW_PRESS;
    }

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
