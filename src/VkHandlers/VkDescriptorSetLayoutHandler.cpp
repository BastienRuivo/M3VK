#include "header/VkHandlers/VkDescriptorSetLayoutHandler.h"
#include "header/ApplicationInfo.h"
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

VkDescriptorSetLayoutHandler::VkDescriptorSetLayoutHandler(std::initializer_list<VkDescriptorSetLayoutBinding> bindings) : VkDescriptorSetLayoutHandler(bindings.begin(), static_cast<uint32_t>(bindings.size())) {}

VkDescriptorSetLayoutHandler::VkDescriptorSetLayoutHandler(const VkDescriptorSetLayoutBinding* bindings, uint32_t bindingCount)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkDescriptorSetLayoutHandler Creation !");
#endif



    VkDescriptorSetLayoutCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = bindingCount,
        .pBindings = bindings
    };

    if(vkCreateDescriptorSetLayout(ApplicationInfo::Device(), &createInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
}

VkDescriptorSetLayoutHandler::~VkDescriptorSetLayoutHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroyDescriptorSetLayout(ApplicationInfo::Device(), _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkDescriptorSetLayoutHandler Destroyed !");
#endif
}

VkDescriptorSetLayoutHandler::VkDescriptorSetLayoutHandler(VkDescriptorSetLayoutHandler && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkDescriptorSetLayoutHandler Move Creation !");
#endif

    _internal = other._internal;

    other._internal = VK_NULL_HANDLE;
}

VkDescriptorSetLayoutHandler& VkDescriptorSetLayoutHandler::operator=(VkDescriptorSetLayoutHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;

        other._internal = VK_NULL_HANDLE;
    }

    return *this;
}
