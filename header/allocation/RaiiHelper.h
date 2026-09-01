#pragma once

#include "GLFW/glfw3.h"
#include "rendering/QueueFamilyIds.h"
#include "vulkan/vulkan.hpp"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace RaiiHelper
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

    vk::raii::Instance MakeInstance(const vk::raii::Context& context,
        const std::string_view name,
        const uint32_t appVersion,
        const uint32_t engineVersion,
        const uint32_t apiVersion);

    vk::raii::SurfaceKHR MakeSurface(const vk::raii::Instance& instance, GLFWwindow* pWindow);
    vk::raii::PhysicalDevice MakePhysicalDevice(const vk::raii::Instance& instance, vk::SurfaceKHR windowSurface, const std::vector<const char *>& deviceExtensions);
    vk::raii::Device MakeDevice(const vk::raii::PhysicalDevice& physicalDevice, vk::SurfaceKHR windowSurface, const std::vector<const char*>& deviceExtensions);
    vk::raii::Sampler MakeSampler(vk::Filter oversampling = vk::Filter::eLinear, vk::Filter undersampling = vk::Filter::eLinear,
        vk::SamplerMipmapMode mipmapMode = vk::SamplerMipmapMode::eLinear,
        bool hasAniso = true);

    vk::raii::ImageView MakeImageView(vk::Image image, vk::Format format, uint32_t mipCount);
    vk::raii::ImageView MakeImageView(vk::Image image, vk::Format format, uint32_t mipCount, vk::ImageAspectFlags aspectMask);
    vk::raii::ImageView MakeImageView(vk::Image image, vk::Format format, uint32_t mipCount, vk::ImageAspectFlags aspectMask, vk::ImageViewType type);
};
