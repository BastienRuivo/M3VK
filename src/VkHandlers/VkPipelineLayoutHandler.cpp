#include "header/VkHandlers/VkPipelineLayoutHandler.h"
#include "header/ApplicationInfo.h"
#include <cstdint>
#include <initializer_list>
#include <span>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

VkPipelineLayoutHandler::VkPipelineLayoutHandler( std::span<const VkDescriptorSetLayout> descriptorLayouts, std::span<const VkPushConstantRange> pushConstantRanges)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkPipelineLayoutHandler Creation !");
#endif


    VkPipelineLayoutCreateInfo layoutCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(descriptorLayouts.size()),
        .pSetLayouts = descriptorLayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
        .pPushConstantRanges = pushConstantRanges.data()
    };

    if(vkCreatePipelineLayout(ApplicationInfo::Device(), &layoutCreateInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create VK Layout !");
    }
}
VkPipelineLayoutHandler::~VkPipelineLayoutHandler()
{
    if(_internal == VK_NULL_HANDLE) return;
    vkDestroyPipelineLayout(ApplicationInfo::Device(), _internal, nullptr);

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

    other._internal = VK_NULL_HANDLE;
}

VkPipelineLayoutHandler& VkPipelineLayoutHandler::operator=(VkPipelineLayoutHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;

        other._internal = VK_NULL_HANDLE;
    }

    return *this;
}
