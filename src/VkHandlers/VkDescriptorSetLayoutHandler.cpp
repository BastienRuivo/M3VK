#include "header/VkHandlers/VkDescriptorSetLayoutHandler.h"
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

VkDescriptorSetLayoutHandler::VkDescriptorSetLayoutHandler(VkDevice device, std::initializer_list<VkDescriptorSetLayoutBinding> bindings) : VkDescriptorSetLayoutHandler(device, bindings.begin(), static_cast<uint32_t>(bindings.size())) {}

VkDescriptorSetLayoutHandler::VkDescriptorSetLayoutHandler(VkDevice device, const VkDescriptorSetLayoutBinding* bindings, uint32_t bindingCount)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkDescriptorSetLayoutHandler Creation !");
#endif

    _device = device;

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = bindingCount;
    createInfo.pBindings = bindings;

    if(vkCreateDescriptorSetLayout(_device, &createInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
}

VkDescriptorSetLayoutHandler::~VkDescriptorSetLayoutHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroyDescriptorSetLayout(_device, _internal, nullptr);

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
    _device = other._device;
    other._internal = VK_NULL_HANDLE;
}

VkDescriptorSetLayoutHandler& VkDescriptorSetLayoutHandler::operator=(VkDescriptorSetLayoutHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        _device = other._device;
        other._internal = VK_NULL_HANDLE;
    }

    return *this;
}
