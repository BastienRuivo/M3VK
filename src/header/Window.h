#ifndef WINDOW_CLASS
#define WINDOW_CLASS


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window
{
    public:
    void init(int width, int height, const char* title, void* resizeObject, GLFWframebuffersizefun resizeCallback);
    void ResizeWindow(int width, int height);
    void Dispose();

    bool ShouldClose() const;
    void ProcessEvent() const;
    void GetFramebufferSize(int& width, int& height) const;
    void CreateWindowSurface(const VkInstance & _instance, VkSurfaceKHR& surface) const;


    private:
    GLFWwindow* _pWindow = nullptr;
    int _width;
    int _height;
    const char * _title;
};

#endif
