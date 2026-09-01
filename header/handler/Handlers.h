#pragma once

#include "GLFW/glfw3.h"
#include "rendering/QueueFamilyIds.h"
#include <cstdint>
#include <span>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <concepts>

template<typename T>
    requires std::same_as<T, vk::Fence>
    || std::same_as<T, vk::ImageView>
    || std::same_as<T, vk::PipelineLayout>
    || std::same_as<T, vk::Sampler>
class Handler
{
public:
    Handler() {}
    virtual ~Handler() {};
    Handler(Handler&& other) noexcept
    {
        _internal = std::exchange(other._internal, T{});
    }
    Handler& operator=(Handler&& other) noexcept
    {
        if(this != &other)
        {
            _internal = std::exchange(other._internal, T{});
        }
        return *this;
    }

    Handler(const Handler&) = delete;
    Handler& operator=(const Handler&) = delete;

    inline T Internal() const { return _internal; }

protected:
    T _internal{};
};


class VkFenceHandler : public Handler<vk::Fence>
{
public:
    VkFenceHandler();
    ~VkFenceHandler() override;

    VkFenceHandler(VkFenceHandler&& other) noexcept = default;
    VkFenceHandler& operator=(VkFenceHandler&& other) noexcept = default;

    void Wait(uint64_t timeout) const;
    void Reset() const;
};

class VkSamplerHandler : public Handler<vk::Sampler>
{
public:
    VkSamplerHandler(vk::Filter oversampling = vk::Filter::eLinear, vk::Filter undersampling = vk::Filter::eLinear,
        vk::SamplerMipmapMode mipmapMode = vk::SamplerMipmapMode::eLinear,
        bool hasAniso = true);
    ~VkSamplerHandler() override;

    VkSamplerHandler(VkSamplerHandler&& other) noexcept = default;
    VkSamplerHandler& operator=(VkSamplerHandler&& other) noexcept = default;
};

class VkPipelineLayoutHandler : public Handler<vk::PipelineLayout>
{
public:
    VkPipelineLayoutHandler(std::span<const vk::DescriptorSetLayout> descriptorLayouts, std::span<const vk::PushConstantRange> pushConstantRanges);
    ~VkPipelineLayoutHandler() override;

    VkPipelineLayoutHandler(VkPipelineLayoutHandler&& other) noexcept = default;
    VkPipelineLayoutHandler& operator=(VkPipelineLayoutHandler&& other) noexcept = default;
};

namespace M3VKConstruct
{
    namespace Helper
    {
        std::vector<const char*> GetRequiredExtensions();

        int ScorePhysicalDeviceSuitability(vk::PhysicalDevice device, vk::SurfaceKHR windowSurface,
            const std::vector<const char *>& deviceExtensions,
            vk::PhysicalDeviceProperties& deviceProperties,
            QueueFamilyIds& familyIds);

            bool CheckPhysicalDeviceExtensionSupport(vk::PhysicalDevice physicalDevice,
            const std::vector<const char *>& deviceExtensions);

        uint32_t FindMemoryType(vk::PhysicalDevice device, uint32_t typeFilter,
            vk::MemoryPropertyFlags properties);
    };

    vk::raii::Instance MakeInstance(const vk::raii::Context& context,
        const std::string_view name,
        const uint32_t appVersion,
        const uint32_t engineVersion,
        const uint32_t apiVersion);

    vk::raii::SurfaceKHR MakeSurface(const vk::raii::Instance& instance, GLFWwindow* pWindow);
    vk::raii::PhysicalDevice MakePhysicalDevice(const vk::raii::Instance& instance, vk::SurfaceKHR windowSurface, const std::vector<const char *>& deviceExtensions);
    vk::raii::Device MakeDevice(const vk::raii::PhysicalDevice& physicalDevice, vk::SurfaceKHR windowSurface, const std::vector<const char*>& deviceExtensions);
};
