#pragma once

#include "GLFW/glfw3.h"
#include <span>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <concepts>

template<typename T>
    requires std::same_as<T, VkCommandPool>
    || std::same_as<T, VkDevice>
    || std::same_as<T, VkPhysicalDevice>
    || std::same_as<T, VkFence>
    || std::same_as<T, VkSemaphore>
    || std::same_as<T, VkImageView>
    || std::same_as<T, VkInstance>
    || std::same_as<T, VkPipeline>
    || std::same_as<T, VkPipelineLayout>
    || std::same_as<T, VkQueue>
    || std::same_as<T, VkSampler>
    || std::same_as<T, VkSurfaceKHR>
class Handler
{
public:
    Handler() {}
    virtual ~Handler() {};
    Handler(Handler&& other) noexcept
    {
        _internal = std::exchange(other._internal, VK_NULL_HANDLE);
    }
    Handler& operator=(Handler&& other) noexcept
    {
        if(this != &other)
        {
            _internal = std::exchange(other._internal, VK_NULL_HANDLE);
        }
        return *this;
    }

    Handler(const Handler&) = delete;
    Handler& operator=(const Handler&) = delete;

    inline T Internal() const { return _internal; }

protected:
    T _internal = VK_NULL_HANDLE;
};

// -- HANDLER DEFINITION

class VkCommandPoolHandler : public Handler<VkCommandPool>
{
public:
    VkCommandPoolHandler();
    ~VkCommandPoolHandler() override;

    VkCommandPoolHandler(VkCommandPoolHandler&& other) noexcept = default;
    VkCommandPoolHandler& operator=(VkCommandPoolHandler&& other) noexcept = default;
};

class VkDeviceHandler : public Handler<VkDevice>
{
public:
    VkDeviceHandler(VkSurfaceKHR windowSurface, const std::vector<const char*>& deviceExtensions);
    ~VkDeviceHandler() override;

    VkDeviceHandler(VkDeviceHandler&& other) noexcept = default;
    VkDeviceHandler& operator=(VkDeviceHandler&& other) noexcept = default;
};

class VkFenceHandler : public Handler<VkFence>
{
public:
    VkFenceHandler();
    ~VkFenceHandler() override;

    VkFenceHandler(VkFenceHandler&& other) noexcept = default;
    VkFenceHandler& operator=(VkFenceHandler&& other) noexcept = default;

    void Wait(uint64_t timeout) const;
    void Reset() const;
};

class VkSemaphoreHandler : public Handler<VkSemaphore>
{
public:
    VkSemaphoreHandler();
    ~VkSemaphoreHandler() override;

    VkSemaphoreHandler(VkSemaphoreHandler&& other) noexcept = default;
    VkSemaphoreHandler& operator=(VkSemaphoreHandler&& other) noexcept = default;
};

class VkInstanceHandler : public Handler<VkInstance>
{
public:
    VkInstanceHandler();
    ~VkInstanceHandler() override;

    VkInstanceHandler(VkInstanceHandler&& other) noexcept = default;
    VkInstanceHandler& operator=(VkInstanceHandler&& other) noexcept = default;
private:
    std::vector<const char*> GetRequiredExtensions() const;
};

class VkPipelineHandler : public Handler<VkPipeline>
{
public:
    VkPipelineHandler(const VkExtent2D& appExtent, VkSampleCountFlagBits msaaSampleCount, VkPipelineLayout pipelineLayout, VkFormat swapChainFormat, VkFormat depthFormat);
    ~VkPipelineHandler() override;

    VkPipelineHandler(VkPipelineHandler&& other) noexcept = default;
    VkPipelineHandler& operator=(VkPipelineHandler&& other) noexcept = default;
};

class VkPipelineLayoutHandler : public Handler<VkPipelineLayout>
{
public:
    VkPipelineLayoutHandler(std::span<const VkDescriptorSetLayout> descriptorLayouts, std::span<const VkPushConstantRange> pushConstantRanges);
    ~VkPipelineLayoutHandler() override;

    VkPipelineLayoutHandler(VkPipelineLayoutHandler&& other) noexcept = default;
    VkPipelineLayoutHandler& operator=(VkPipelineLayoutHandler&& other) noexcept = default;
};

class VkQueueHandler : public Handler<VkQueue>
{
public:

    enum QueueTypeEnum
    {
        Graphics,
        Present,
        Copy
    };
    VkQueueHandler(QueueTypeEnum queueType);
    ~VkQueueHandler() override;

    VkQueueHandler(VkQueueHandler&& other) noexcept;
    VkQueueHandler& operator=(VkQueueHandler&& other) noexcept;

    inline void WaitForIdle() const { vkQueueWaitIdle(_internal); }
    inline uint32_t QueueFamilyId() const { return _queueFamilyIndex; }
    inline QueueTypeEnum QueueType() const { return _type; }

    private:
    uint32_t _queueFamilyIndex;
    enum QueueTypeEnum _type;
};

class VkSamplerHandler : public Handler<VkSampler>
{
public:
    VkSamplerHandler();
    ~VkSamplerHandler() override;

    VkSamplerHandler(VkSamplerHandler&& other) noexcept = default;
    VkSamplerHandler& operator=(VkSamplerHandler&& other) noexcept = default;
};

class VkSurfaceHandler : public Handler<VkSurfaceKHR>
{
public:
    VkSurfaceHandler(VkInstance instance, GLFWwindow* pWindow);
    ~VkSurfaceHandler() override;

    VkSurfaceHandler(VkSurfaceHandler&& other) noexcept = default;
    VkSurfaceHandler& operator=(VkSurfaceHandler&& other) noexcept = default;
    private:
    VkInstance _instance;
};
