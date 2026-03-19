#include "header/VkHandlers/VkPipelineLayoutHandler.h"
#include "header/DebugLayer.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

VkPipelineLayoutHandler::VkPipelineLayoutHandler(VkDevice device, VkDescriptorSetLayout descriptorLayout)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkPipelineLayoutHandler Creation !");
#endif
    VkDescriptorSetLayout descriptorSetLayout = descriptorLayout;

    _device = device;

    // Uniforms (empty for now)
    VkPipelineLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = 1;
    layoutCreateInfo.pSetLayouts = &descriptorSetLayout;
    layoutCreateInfo.pushConstantRangeCount = 0; // Optional
    layoutCreateInfo.pPushConstantRanges = nullptr; // Optional

    if(vkCreatePipelineLayout(_device, &layoutCreateInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create VK Layout !");
    }
}
VkPipelineLayoutHandler::~VkPipelineLayoutHandler()
{
    if(_internal == VK_NULL_HANDLE) return;
    vkDestroyPipelineLayout(_device, _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkPipelineLayoutHandler Destroyed !");
#endif
}

VkPipelineLayoutHandler::VkPipelineLayoutHandler(VkPipelineLayoutHandler && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkPipelineLayoutHandler Move Creation !");
#endif

    _internal = other._internal;
    _device = other._device;
    other._internal = VK_NULL_HANDLE;
}

VkPipelineLayoutHandler& VkPipelineLayoutHandler::operator=(VkPipelineLayoutHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        _device = other._device;
        other._internal = VK_NULL_HANDLE;
    }

    return *this;
}
