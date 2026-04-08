#include "header/ApplicationInfo.h"
#include <vulkan/vulkan_core.h>

VkSampleCountFlagBits ApplicationInfo::GetMaxUsableSampleCount(VkSampleCountFlagBits maxSample) const
{
    VkSampleCountFlags counts = _properties.limits.framebufferColorSampleCounts & _properties.limits.framebufferDepthSampleCounts;

    if((counts & VK_SAMPLE_COUNT_64_BIT) && (maxSample >= VK_SAMPLE_COUNT_64_BIT)) return VK_SAMPLE_COUNT_64_BIT;
    else if((counts & VK_SAMPLE_COUNT_32_BIT) && (maxSample >= VK_SAMPLE_COUNT_32_BIT)) return VK_SAMPLE_COUNT_32_BIT;
    else if((counts & VK_SAMPLE_COUNT_16_BIT) && (maxSample >= VK_SAMPLE_COUNT_16_BIT)) return VK_SAMPLE_COUNT_16_BIT;
    else if((counts & VK_SAMPLE_COUNT_8_BIT) && (maxSample >= VK_SAMPLE_COUNT_8_BIT)) return VK_SAMPLE_COUNT_8_BIT;
    else if((counts & VK_SAMPLE_COUNT_4_BIT) && (maxSample >= VK_SAMPLE_COUNT_4_BIT)) return VK_SAMPLE_COUNT_4_BIT;
    else if((counts & VK_SAMPLE_COUNT_2_BIT) && (maxSample >= VK_SAMPLE_COUNT_2_BIT)) return VK_SAMPLE_COUNT_2_BIT;
    else return VK_SAMPLE_COUNT_1_BIT;
}

void ApplicationInfo::SetPhysicalDeviceInformation(VkPhysicalDeviceProperties properties, const ProjectHelper::QueueFamilyIds& queueFamilyIds)
{
    _properties = properties;
    _queueFamilyIds = queueFamilyIds;
    _msaaSample = GetMaxUsableSampleCount(Constant::MaxMSAASample);
}
