#include "application/ApplicationInfo.h"
#include "application/DebugLayer.h"
#include "rendering/QueueFamilyIds.h"
#include <cstdint>
#include <vulkan/vulkan.hpp>

vk::SampleCountFlagBits ApplicationInfo::GetMaxUsableSampleCount(vk::SampleCountFlagBits maxSample) const
{
    vk::SampleCountFlags counts = _properties.limits.framebufferColorSampleCounts & _properties.limits.framebufferDepthSampleCounts;

    if((counts & vk::SampleCountFlagBits::e64) && (maxSample >= vk::SampleCountFlagBits::e64)) return vk::SampleCountFlagBits::e64;
    else if((counts & vk::SampleCountFlagBits::e32) && (maxSample >= vk::SampleCountFlagBits::e32)) return vk::SampleCountFlagBits::e32;
    else if((counts & vk::SampleCountFlagBits::e16) && (maxSample >= vk::SampleCountFlagBits::e16)) return vk::SampleCountFlagBits::e16;
    else if((counts & vk::SampleCountFlagBits::e8) && (maxSample >= vk::SampleCountFlagBits::e8)) return vk::SampleCountFlagBits::e8;
    else if((counts & vk::SampleCountFlagBits::e4) && (maxSample >= vk::SampleCountFlagBits::e4)) return vk::SampleCountFlagBits::e4;
    else if((counts & vk::SampleCountFlagBits::e2) && (maxSample >= vk::SampleCountFlagBits::e2)) return vk::SampleCountFlagBits::e2;
    else return vk::SampleCountFlagBits::e1;
}



void ApplicationInfo::VRAMAllocate(size_t size, AllocType aType)
{
    //DebugLayer::Log(DebugLayer::INFO, "Allocating " + std::to_string(size) + " Of type " + AllocTypeNames[aType]);
    ApplicationInfo::Get()._currentVRAM = ApplicationInfo::Get()._currentVRAM + size;
}

void ApplicationInfo::VRAMRelease(size_t size, AllocType aType)
{
    //DebugLayer::Log(DebugLayer::INFO, "Releasing " + std::to_string(size) + " Of type " + AllocTypeNames[aType]);
    ApplicationInfo::Get()._currentVRAM = ApplicationInfo::Get()._currentVRAM - size;
}

uint32_t ApplicationInfo::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memoryProperties = Get()._physicalDevice->getMemoryProperties();

    for (uint32_t memoryType = 0; memoryType < memoryProperties.memoryTypeCount; ++memoryType)
    {
        // is suitable for buffer & writable by CPU
        if((typeFilter & (1 << memoryType)) && ((memoryProperties.memoryTypes[memoryType].propertyFlags & properties) == properties))
        {
            return memoryType;
        }
    }

    DebugLayer::Log(DebugLayer::LogType::WARNING, "Can't find suitable memory type for flags {" + vk::to_string(properties) +"}");
    return UINT32_MAX;
}

ApplicationInfo::Initializer::Initializer(const vk::raii::Instance& instance,
    const vk::raii::SurfaceKHR& surface,
    const vk::raii::PhysicalDevice& physicalDevice,
    const vk::raii::Device& device)
{
    ApplicationInfo::Get()._vkInstance = &instance;
    ApplicationInfo::Get()._surface = &surface;
    ApplicationInfo::Get()._physicalDevice = &physicalDevice;
    ApplicationInfo::Get()._device = &device;
    ApplicationInfo::Get()._properties = physicalDevice.getProperties();
    ApplicationInfo::Get()._queueFamilyIds = QueueFamilyIds::QueryQueueFamilies(physicalDevice, surface);
    ApplicationInfo::Get()._msaaSample = ApplicationInfo::Get().GetMaxUsableSampleCount(Constant::MaxMSAASample);
}
