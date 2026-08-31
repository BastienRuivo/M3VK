#pragma once

#include <cstddef>
#include <cstdint>
#include <sys/stat.h>
#include <sys/types.h>
#include <vulkan/vulkan.hpp>
#include "rendering/QueueFamilyIds.h"
#include "handler/Handlers.h"

class ApplicationInfo
{
    public:

    class Initializer
    {
        public:
        Initializer() = delete;
        Initializer(const vk::raii::Instance& instance,
            const vk::raii::SurfaceKHR& surface,
            const vk::raii::PhysicalDevice& physicalDevice,
            const vk::raii::Device& device);
    };

    ApplicationInfo(const ApplicationInfo&) = delete;
    void operator=(const ApplicationInfo&) = delete;

    static ApplicationInfo& Get()
    {
        static ApplicationInfo instance;
        return instance;
    }

    static inline vk::Instance Instance() { return *ApplicationInfo::Get()._vkInstance; }
    static inline vk::SurfaceKHR Surface() { return *ApplicationInfo::Get()._surface; }
    static inline vk::PhysicalDevice PhysicalDevice() { return *ApplicationInfo::Get()._physicalDevice; }
    static inline vk::Device Device() { return *ApplicationInfo::Get()._device; }

    static inline const vk::raii::Instance& RaiiInstance() { return *ApplicationInfo::Get()._vkInstance; }
    static inline const vk::raii::SurfaceKHR& RaiiSurface() { return *ApplicationInfo::Get()._surface; }
    static inline const vk::raii::PhysicalDevice& RaiiPhysicalDevice() { return *ApplicationInfo::Get()._physicalDevice; }
    static inline const vk::raii::Device& RaiiDevice() { return *ApplicationInfo::Get()._device; }

    struct Constant
    {
        static inline constexpr vk::SampleCountFlagBits MaxMSAASample = vk::SampleCountFlagBits::e8;
        static inline constexpr uint32_t MaxFrameInFlight = 2;
        static inline constexpr size_t VertexBufferMaxSize = 16777216; // 2^23
        static inline constexpr size_t IndexBufferMaxSize = 16777216;
        static inline constexpr size_t DrawIndirectBufferMaxSize = 16777216;
        static inline constexpr size_t MaterialBufferMaxSize = 2048;
        static inline constexpr vk::Format DepthFormat = vk::Format::eD32Sfloat;
        static inline constexpr uint32_t MaxBindlessTextureCount = 1024;
        static inline constexpr uint32_t MaxMipCount = 16;
    };

    static inline const QueueFamilyIds& GetQueueFamilyIds() { return ApplicationInfo::Get()._queueFamilyIds; }
    static inline uint32_t GetGraphicsQueueId() { return ApplicationInfo::Get()._queueFamilyIds.GraphicsCompute.value(); }
    static inline uint32_t GetPresentQueueId() { return ApplicationInfo::Get()._queueFamilyIds.Present.value(); }
    static inline uint32_t GetTransferQueueId() { return ApplicationInfo::Get()._queueFamilyIds.Transfer.value(); }
    static inline const vk::PhysicalDeviceProperties& GetProperties() { return ApplicationInfo::Get()._properties; }
    static inline vk::SampleCountFlagBits GetMsaaSample()    { return ApplicationInfo::Get()._msaaSample; }

    static uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    static inline uint32_t CurrentFrame() { return (ApplicationInfo::Get()._currentFrame) % Constant::MaxFrameInFlight; }
    static inline uint32_t PreviousFrame() { return (ApplicationInfo::Get()._currentFrame + Constant::MaxFrameInFlight - 1) % Constant::MaxFrameInFlight; }
    static inline void NextFrame() { ApplicationInfo::Get()._currentFrame++; }
    static inline uint32_t GetFrameCount() { return ApplicationInfo::Get()._currentFrame; }
    static inline size_t GetVRAMUsage() { return ApplicationInfo::Get()._currentVRAM; }

    enum AllocType
    {
        Buffer,
        Image
    };
    static constexpr std::string AllocTypeNames[4]
    {
        "Buffer",
        "Image",
    };
    static void VRAMAllocate(size_t size, AllocType aType);
    static void VRAMRelease(size_t size, AllocType aType);

    private:
    vk::SampleCountFlagBits GetMaxUsableSampleCount(vk::SampleCountFlagBits maxSample) const;
    ApplicationInfo() {}

    const vk::raii::Instance* _vkInstance = nullptr;
    const vk::raii::SurfaceKHR* _surface = nullptr;
    const vk::raii::PhysicalDevice* _physicalDevice = nullptr;
    const vk::raii::Device* _device = nullptr;

    QueueFamilyIds _queueFamilyIds;
    vk::PhysicalDeviceProperties _properties;
    vk::SampleCountFlagBits  _msaaSample = vk::SampleCountFlagBits::e1;
    uint32_t _currentFrame = 0;
    uint32_t _currentVRAM = 0;

    friend class VkQueueHandler;
};
