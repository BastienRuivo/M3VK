#pragma once

#include "header/ProjectHelper.h"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

class VkPhysicalDeviceHandler
{
    public:

    const VkSampleCountFlagBits MaxMSAASample = VK_SAMPLE_COUNT_8_BIT;

    VkPhysicalDeviceHandler(VkInstance instance, VkSurfaceKHR windowSurface, const std::vector<const char *>& deviceExtensions);
    ~VkPhysicalDeviceHandler();

    inline VkPhysicalDevice Get() const
    {
        return _internal;
    }

    int ScoreDeviceSuitability(VkPhysicalDevice physicalDevice, VkSurfaceKHR windowSurface, const std::vector<const char *>& deviceExtensions, VkPhysicalDeviceProperties& deviceProperties, ProjectHelper::QueueFamilyIds& familyIds) const;
    bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice, const std::vector<const char *>& deviceExtensions) const;
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    VkPhysicalDeviceHandler(VkPhysicalDeviceHandler&& other) noexcept;
    VkPhysicalDeviceHandler& operator=(VkPhysicalDeviceHandler&& other) noexcept;

    VkPhysicalDeviceHandler(const VkPhysicalDeviceHandler&) = delete;
    VkPhysicalDeviceHandler& operator=(const VkPhysicalDeviceHandler&) = delete;

    inline const ProjectHelper::QueueFamilyIds& GetQueueFamilyIds() const
    {
        return _queueFamilyIds;
    }

    inline uint32_t GetGraphicsQueueId() const
    {
        return _queueFamilyIds.Graphics.value();
    }

    inline uint32_t GetPresentQueueId() const
    {
        return _queueFamilyIds.Present.value();
    }

    inline const VkPhysicalDeviceProperties& GetProperties() const
    {
        return _properties;
    }

    inline VkSampleCountFlagBits GetMsaaSample() const
    {
        return _msaaSample;
    }

    private:
    VkSampleCountFlagBits GetMaxUsableSampleCount(VkSampleCountFlagBits maxSample) const;
    VkPhysicalDevice _internal = VK_NULL_HANDLE;
    ProjectHelper::QueueFamilyIds _queueFamilyIds{};
    VkPhysicalDeviceProperties _properties{};
    VkSampleCountFlagBits  _msaaSample = VK_SAMPLE_COUNT_1_BIT;
};
