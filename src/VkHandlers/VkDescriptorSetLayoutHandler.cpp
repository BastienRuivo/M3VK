#include "header/VkHandlers/VkDescriptorSetLayoutHandler.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

VkDescriptorSetLayoutHandler::VkDescriptorSetLayoutHandler(VkDevice device)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkDescriptorSetLayoutHandler Creation !");
#endif

    _device = device;

    VkDescriptorSetLayoutBinding cameraDataLayoutBindingDescriptor{};
    cameraDataLayoutBindingDescriptor.binding = 0;
    cameraDataLayoutBindingDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraDataLayoutBindingDescriptor.descriptorCount = 1;
    cameraDataLayoutBindingDescriptor.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    cameraDataLayoutBindingDescriptor.pImmutableSamplers = nullptr; // image sampling

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = 1;
    createInfo.pBindings = &cameraDataLayoutBindingDescriptor;

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
