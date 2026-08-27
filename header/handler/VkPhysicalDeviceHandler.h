#pragma once

#include "rendering/QueueFamilyIds.h"
#include "handler/Handlers.h"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.hpp>

class VkPhysicalDeviceHandler : public Handler<vk::PhysicalDevice>
{
    public:

    VkPhysicalDeviceHandler(vk::Instance instance, vk::SurfaceKHR windowSurface, const std::vector<const char *>& deviceExtensions);
    ~VkPhysicalDeviceHandler() override;

    VkPhysicalDeviceHandler(VkPhysicalDeviceHandler&& other) noexcept = default;
    VkPhysicalDeviceHandler& operator=(VkPhysicalDeviceHandler&& other) noexcept = default;

    int ScoreDeviceSuitability(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR windowSurface, const std::vector<const char *>& deviceExtensions, vk::PhysicalDeviceProperties& deviceProperties, QueueFamilyIds& familyIds) const;
    bool CheckDeviceExtensionSupport(vk::PhysicalDevice physicalDevice, const std::vector<const char *>& deviceExtensions) const;
    uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

    friend class ApplicationInfo;
};
