#pragma once

#include <cstdint>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>

#include "header/ProjectHelper.h"
#include "header/VkHandlers/VkPhysicalDeviceHandler.h"

class ApplicationInfo
{
    public:

    ApplicationInfo(const ApplicationInfo&) = delete;
    void operator=(const ApplicationInfo&) = delete;

    static ApplicationInfo& Get()
    {
        static ApplicationInfo instance;
        return instance;
    }

    struct Constant
    {
        static inline constexpr VkSampleCountFlagBits MaxMSAASample = VK_SAMPLE_COUNT_8_BIT;
        static inline constexpr uint32_t MaxFrameInCount = 2;
        static inline constexpr uint32_t VertexBufferMaxSize = 8388608; // 2^23
        static inline constexpr uint32_t IndexBufferMaxSize = 8388608;
        static inline constexpr VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT;
    };

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

    void SetPhysicalDeviceInformation(VkPhysicalDeviceProperties properties, const ProjectHelper::QueueFamilyIds& queueFamilyIds);

    private:
    ApplicationInfo() {}
    VkSampleCountFlagBits GetMaxUsableSampleCount(VkSampleCountFlagBits maxSample) const;
    ProjectHelper::QueueFamilyIds _queueFamilyIds;
    VkPhysicalDeviceProperties _properties;
    VkSampleCountFlagBits  _msaaSample = VK_SAMPLE_COUNT_1_BIT;
};
