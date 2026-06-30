#include "application/Application.h"
#include "application/Pipeline.h"
#include "application/ApplicationInfo.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GPUImage.h"
#include "rendering/MultiFrame.h"
#include "rendering/SwapChain.h"
#include "application/DebugLayer.h"
#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdint>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/trigonometric.hpp>
#include <memory>
#include <stdexcept>
#include "asset/CPUImage.h"

#ifdef M3VK_VERBOSE_LOG
#include <string>
#endif

#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void Application::ResizeCallback(GLFWwindow* window, int width, int height)
{
    Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->UpdateWindowSize(width, height);
}

void Application::WindowFocusCallback(GLFWwindow* window, int focused)
{
    Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

    if(focused)
    {
        app->_inputDeltaPrevent = 3;
    }
}

void Application::MouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
{
    Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

    if(app->_inputDeltaPrevent > 0)
    {
        app->_lastMouseX = xpos;
        app->_lastMouseY = ypos;
        app->_inputDeltaPrevent--;
        return;
    }

    double dx = xpos - app->_lastMouseX;
    double dy = ypos - app->_lastMouseY;

    if(!app->_mouseLocked)
    {
        dx = dy = 0.0f;
    }

    const float sensitivity = 0.1f;

    app->_pipeline.OnMouseMove(dx * sensitivity, dy * sensitivity);

    app->_lastMouseX = xpos;
    app->_lastMouseY = ypos;
}

void Application::UpdateWindowSize(int width, int height)
{
    _window.ResizeWindow(width, height);
}

void Application::RefreshSwapChain()
{
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_window.Internal(), &width, &height);
        glfwPollEvents();
    }

    vkDeviceWaitIdle(ApplicationInfo::Device());

    // TODO : Swap chain is currently resetted the first frame beacause it is out of date
    _swapChain.reset();
    _swapChain = std::make_unique<SwapChain>(_window, _windowSurface.Internal());

    auto extents = _swapChain->GetExtent();
    _pipeline.Refresh(extents);
}

void Application::DrawFrame()
{
    uint32_t currentFrame = ApplicationInfo::CurrentFrame();
    _waitFence.Get(currentFrame).Wait(UINT64_MAX);

    // Acquire image to draw on
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(ApplicationInfo::Device(), _swapChain->Internal(), UINT64_MAX, _availableImageSemaphore.Internal(currentFrame), VK_NULL_HANDLE, &imageIndex);
    ApplicationInfo::NextFrame();
    if(result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RefreshSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Only reset the fence if we are submitting work
    _waitFence.Get(currentFrame).Reset();

    // UI
    _userInterface.StartFrame();
    _pipeline.DoUI(_userInterface);
    _userInterface.Render();

    _pipeline.UpdateCameraData(_swapChain->GetExtent());

    const CommandBuffer& commandBuffer = _commandBuffer.Get(currentFrame);

    commandBuffer.Reset();
    _pipeline.Execute(commandBuffer, *_swapChain, _userInterface, imageIndex);

    // stackallocs that can be cached.
    VkSemaphore wait[] = {_availableImageSemaphore.Internal(currentFrame)};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphore[] = {_renderFinishedSemaphores.Internal(imageIndex)};

    commandBuffer.Submit(wait, 1, waitStages, signalSemaphore, 1, _waitFence.Internal(currentFrame));

    // actually present the frame
    VkSwapchainKHR swapChain = _swapChain->Internal();
    VkPresentInfoKHR presentInfo
    {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &swapChain,
        .pImageIndices = &imageIndex
    };

    result = vkQueuePresentKHR(_graphicsComputeQueue.Internal(), &presentInfo);

    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        RefreshSwapChain();
    }
    else if(result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    ApplicationInfo::NextFrame();
}

Application::Application() :
    // Core Window & Instance
    _window(1920, 1080, "Window", this, Application::ResizeCallback, Application::MouseMoveCallback, Application::WindowFocusCallback),
    _instance(),
    _vkDebugLayer(_instance.Internal()),
    _windowSurface(_instance.Internal(), _window.Internal()),
    _physicalDevice(_instance.Internal(), _windowSurface.Internal(), _deviceExtensions),
    _device(_instance.Internal(), _windowSurface.Internal(), _deviceExtensions),

    // Queues & Swapchain
    _graphicsComputeQueue(VkQueueHandler::Graphics),
    _presentQueue(VkQueueHandler::Present),
    _swapChain(std::make_unique<SwapChain>(_window, _windowSurface.Internal())),

    // Command pool
    _graphicsCommandPool(ApplicationInfo::GetGraphicsQueueId()),
    // Geometry & Data Buffers
    _commandBuffer(ApplicationInfo::Constant::MaxFrameInFlight, _graphicsCommandPool.Internal(), _graphicsComputeQueue.Internal()),
    _pipeline(*_swapChain, _graphicsCommandPool.Internal(), _graphicsComputeQueue.Internal()),

    // Synchronization
    _availableImageSemaphore(ApplicationInfo::Constant::MaxFrameInFlight),
    _renderFinishedSemaphores(_swapChain->Images.Size()),
    _waitFence(ApplicationInfo::Constant::MaxFrameInFlight),
    _userInterface(_window.Internal(), *_swapChain, _graphicsComputeQueue.Internal(), _graphicsCommandPool.Internal())
{
    _window.LockMouse(_mouseLocked);
    {
        CPUImage logo("data/logo.png", STBI_rgb_alpha);
        _window.SetIcon(logo.Data(), logo.Width(), logo.Height());
    }
}

void Application::MainLoop()
{
    bool shouldClose = false;
    while (!_window.ShouldClose() && !shouldClose)
    {
        _window.ProcessEvent();

        auto currentTime = std::chrono::high_resolution_clock::now();

        auto deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - _lastFrameTime).count();
        _lastFrameTime = currentTime;

        _pipeline.DoKeyboardInput(_window, deltaTime);

        if(_window.IsKeyPressed(GLFW_KEY_ESCAPE)) shouldClose = true;

        if(_inputPrevent <= 0)
        {
            if(_window.IsKeyPressed(GLFW_KEY_LEFT_ALT)) _window.LockMouse(_mouseLocked = !_mouseLocked); _inputPrevent = glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate;
        }
        else
        {
            // it's in ms, deduce delta time in ms
            _inputPrevent = _inputPrevent - deltaTime * 1000;
        }

        DrawFrame();
    }

    vkDeviceWaitIdle(ApplicationInfo::Device());
}

Application::~Application()
{
    _swapChain.reset();
}

void Application::Run()
{
    MainLoop();
}
