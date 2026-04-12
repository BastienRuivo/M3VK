#include "header/VkHandlers/VkDescriptorPoolHandler.h"
#include <cstdint>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

#include <stdexcept>
#include <vulkan/vulkan_core.h>

VkDescriptorPoolHandler::VkDescriptorPoolHandler(VkDevice device, std::initializer_list<VkDescriptorPoolSize> poolSizes, uint32_t maxSets) : VkDescriptorPoolHandler(device, poolSizes.begin(), poolSizes.size(), maxSets) {}

VkDescriptorPoolHandler::VkDescriptorPoolHandler(VkDevice device, const VkDescriptorPoolSize* poolSizes, uint32_t poolSizesCount, uint32_t maxSets)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkDescriptorPoolHandler Creation !");
#endif
    _device = device;

    VkDescriptorPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.poolSizeCount = poolSizesCount;
    poolCreateInfo.pPoolSizes = poolSizes;
    poolCreateInfo.maxSets = maxSets;

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
