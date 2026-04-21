#pragma once

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

    inline GLFWwindow* Internal() const
    {
        return _internal;
    }

    void LockMouse(bool lock);

    private:
    GLFWwindow* _internal = nullptr;
    int _width = 0;
    int _height = 0;
    const char * _title = nullptr;
};
