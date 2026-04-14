#include "header/VkHandlers/VkDescriptorPoolHandler.h"
#include "header/ApplicationInfo.h"
#include <cstdint>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

#include <stdexcept>
#include <vulkan/vulkan_core.h>

VkDescriptorPoolHandler::VkDescriptorPoolHandler(std::initializer_list<VkDescriptorPoolSize> poolSizes, uint32_t maxSets) : VkDescriptorPoolHandler(poolSizes.begin(), poolSizes.size(), maxSets) {}

VkDescriptorPoolHandler::VkDescriptorPoolHandler(const VkDescriptorPoolSize* poolSizes, uint32_t poolSizesCount, uint32_t maxSets)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkDescriptorPoolHandler Creation !");
#endif

    VkDescriptorPoolCreateInfo poolCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = maxSets,
        .poolSizeCount = poolSizesCount,
        .pPoolSizes = poolSizes,
    };

    if(vkCreateDescriptorPool(ApplicationInfo::Device(), &poolCreateInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("VK Create Descriptor Pool Failed !");
    }
}

VkDescriptorPoolHandler::~VkDescriptorPoolHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroyDescriptorPool(ApplicationInfo::Device(), _internal, nullptr);

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

    other._internal = VK_NULL_HANDLE;
}

VkDescriptorPoolHandler& VkDescriptorPoolHandler::operator=(VkDescriptorPoolHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;

        other._internal = VK_NULL_HANDLE;
    }

    return *this;
}
