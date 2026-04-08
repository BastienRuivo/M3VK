#include "header/VkHandlers/VkDescriptorPoolHandler.h"
#include <array>
#include <cstdint>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

#include <stdexcept>
#include <vulkan/vulkan_core.h>

VkDescriptorPoolHandler::VkDescriptorPoolHandler(VkDevice device, uint32_t frameCount)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkDescriptorPoolHandler Creation !");
#endif
    _device = device;

    std::array<VkDescriptorPoolSize, 3> poolSizes
    {
        VkDescriptorPoolSize
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = frameCount
        },
        VkDescriptorPoolSize
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = frameCount
        },
        VkDescriptorPoolSize
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = frameCount
        }
    };

    VkDescriptorPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolCreateInfo.pPoolSizes = poolSizes.data();
    poolCreateInfo.maxSets = frameCount;

    if(vkCreateDescriptorPool(_device, &poolCreateInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("VK Create Descriptor Pool Failed !");
    }
}

VkDescriptorPoolHandler::~VkDescriptorPoolHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroyDescriptorPool(_device, _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkDescriptorPoolHandler Destroyed !");
#endif
}

VkDescriptorPoolHandler::VkDescriptorPoolHandler(VkDescriptorPoolHandler && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkDescriptorPoolHandler Move Creation !");
#endif

    _internal = other._internal;
    _device = other._device;
    other._internal = VK_NULL_HANDLE;
}

VkDescriptorPoolHandler& VkDescriptorPoolHandler::operator=(VkDescriptorPoolHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        _device = other._device;
        other._internal = VK_NULL_HANDLE;
    }

    return *this;
}
