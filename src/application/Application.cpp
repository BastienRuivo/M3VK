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

#include <vulkan/vulkan.hpp>

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

    ApplicationInfo::Device().waitIdle();

    // TODO : Swap chain is currently resetted the first frame beacause it is out of date
    _swapChain.reset();
    _swapChain = std::make_unique<SwapChain>(_window, _windowSurface);

    auto extents = _swapChain->GetExtent();
    CommandBuffer cmdBuffer(_graphicsCommandPool.Internal(), _graphicsComputeQueue);
    cmdBuffer.BeginSingleTime();
    {
        _pipeline.Refresh(cmdBuffer, extents);
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
}

void Application::DrawFrame()
{
    uint32_t currentFrame = ApplicationInfo::CurrentFrame();
    _waitFence.Get(currentFrame).Wait(UINT64_MAX);

    vk::AcquireNextImageInfoKHR acquireInfo = vk::AcquireNextImageInfoKHR{}
        .setSwapchain(_swapChain->Internal())
        .setTimeout(UINT32_MAX) // 4sec = timeout, just a test for now but seems rationnal ?
        .setSemaphore(_availableImageSemaphore.Internal(currentFrame))
        .setFence(nullptr)
        .setDeviceMask(1u);

    // Use the raw-pointer, non-throwing overload: the enhanced ResultValue-returning acquireNextImage2KHR
    // throws vk::OutOfDateKHRError instead of returning eErrorOutOfDateKHR as a value (unless
    // VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS is defined), which would skip the manual check below.
    uint32_t imageIndex;
    vk::Result acquireResult = ApplicationInfo::Device().acquireNextImage2KHR(&acquireInfo, &imageIndex);

    if(acquireResult == vk::Result::eErrorOutOfDateKHR)
    {
        RefreshSwapChain();
        return;
    }
    else if (acquireResult != vk::Result::eSuccess && acquireResult != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Only reset the fence if we are submitting work
    _waitFence.Get(currentFrame).Reset();

    // UI
    _userInterface.StartFrame();
    _pipeline.DoUI(_userInterface);
    _userInterface.Render();

    _pipeline.PreRender(_swapChain->GetExtent());

    const CommandBuffer& commandBuffer = _commandBuffer.Get(currentFrame);

    commandBuffer.Reset();
    _pipeline.Execute(commandBuffer, *_swapChain, _userInterface, imageIndex);

    vk::SemaphoreSubmitInfo waitSemaphore = _availableImageSemaphore.Get(currentFrame).GetSubmitInfo(vk::PipelineStageFlagBits2::eColorAttachmentOutput);
    vk::SemaphoreSubmitInfo signalSemaphore = _renderFinishedSemaphores.Get(imageIndex).GetSubmitInfo(vk::PipelineStageFlagBits2::eAllGraphics);

    commandBuffer.Submit({&waitSemaphore, 1}, {&signalSemaphore, 1}, _waitFence.Internal(currentFrame));

    // actually present the frame
    vk::SwapchainKHR swapChain = _swapChain->Internal();
    vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR{}
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&signalSemaphore.semaphore)
        .setSwapchainCount(1)
        .setPSwapchains(&swapChain)
        .setPImageIndices(&imageIndex);

    auto presentRes = _graphicsComputeQueue.presentKHR(presentInfo);

    if(presentRes == vk::Result::eErrorOutOfDateKHR || presentRes == vk::Result::eSuboptimalKHR)
    {
        RefreshSwapChain();
    }
    else if(presentRes != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    ApplicationInfo::NextFrame();
}

Application::Application() :
    // Core Window & Instance
    _window(1920, 1080, "Window", this, Application::ResizeCallback, Application::MouseMoveCallback, Application::WindowFocusCallback),
    _context(vkGetInstanceProcAddr),
    _instance(M3VKConstruct::MakeInstance(_context, "M3VK", vk::makeVersion(0, 1, 0), vk::makeVersion(0, 1, 0), vk::ApiVersion14)),
    _vkDebugLayer(_instance),
    _windowSurface(M3VKConstruct::MakeSurface(_instance, _window.Internal())),
    _physicalDevice(M3VKConstruct::MakePhysicalDevice(_instance, _windowSurface, _deviceExtensions)),
    _device(M3VKConstruct::MakeDevice(_physicalDevice, _windowSurface, _deviceExtensions)),

    _appInfoInitializer(_instance, _windowSurface, _physicalDevice, _device),

    // Queues & Swapchain
    _graphicsComputeQueue(vk::raii::Queue(ApplicationInfo::RaiiDevice(), ApplicationInfo::Device().getQueue(ApplicationInfo::GetGraphicsQueueId(), 0))),
    _presentQueue(vk::raii::Queue(ApplicationInfo::RaiiDevice(), ApplicationInfo::Device().getQueue(ApplicationInfo::GetPresentQueueId(), 0))),
    _swapChain(std::make_unique<SwapChain>(_window, _windowSurface)),

    // Command pool
    _graphicsCommandPool(ApplicationInfo::GetGraphicsQueueId()),
    // Geometry & Data Buffers
    _commandBuffer(ApplicationInfo::Constant::MaxFrameInFlight, _graphicsCommandPool.Internal(), _graphicsComputeQueue),
    _pipeline(*_swapChain, _graphicsCommandPool.Internal(), _graphicsComputeQueue),

    // Synchronization
    _availableImageSemaphore(ApplicationInfo::Constant::MaxFrameInFlight),
    _renderFinishedSemaphores(_swapChain->Images.Size()),
    _waitFence(ApplicationInfo::Constant::MaxFrameInFlight),
    _userInterface(_window.Internal(), *_swapChain, _graphicsComputeQueue, _graphicsCommandPool.Internal())
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

    ApplicationInfo::Device().waitIdle();
}

Application::~Application()
{
    _swapChain.reset();
}

void Application::Run()
{
    MainLoop();
}
