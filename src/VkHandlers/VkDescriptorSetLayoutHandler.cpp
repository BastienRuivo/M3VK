#include "header/VkHandlers/VkDescriptorSetLayoutHandler.h"
#include "header/DebugLayer.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

VkDescriptorSetLayoutHandler::VkDescriptorSetLayoutHandler(VkDevice device)
{
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkDescriptorSetLayoutHandler Creation !");

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

VkDescriptorSetLayout VkDescriptorSetLayoutHandler::Get() const
{
    return _internal;
}

VkDescriptorSetLayoutHandler::~VkDescriptorSetLayoutHandler()
{
    vkDestroyDescriptorSetLayout(_device, _internal, nullptr);
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkDescriptorSetLayoutHandler Destroyed !");
}
