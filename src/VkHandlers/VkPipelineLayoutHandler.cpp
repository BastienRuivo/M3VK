#include "header/VkHandlers/VkPipelineLayoutHandler.h"
#include "header/DebugLayer.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

VkPipelineLayoutHandler::VkPipelineLayoutHandler(VkDevice device, VkDescriptorSetLayout descriptorLayout)
{
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkPipelineLayoutHandler Creation !");
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

VkPipelineLayout VkPipelineLayoutHandler::Get() const
{
    return _internal;
}

VkPipelineLayoutHandler::~VkPipelineLayoutHandler()
{
    vkDestroyPipelineLayout(_device, _internal, nullptr);
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkPipelineLayoutHandler Destroyed !");
}
