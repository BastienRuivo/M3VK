#include "header/VkHandlers/VkFramebufferHandler.h"
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

VkFramebufferHandler::VkFramebufferHandler(VkDevice device, VkRenderPass renderPass, VkExtent2D imageExtent, std::vector<VkImageView> imageViews)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkFramebufferHandler Creation !");
#endif

    _device = device;

    VkFramebufferCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderPass,
        .attachmentCount = static_cast<uint32_t>(imageViews.size()),
        .pAttachments = imageViews.data(),
        .width = imageExtent.width,
        .height = imageExtent.height,
        .layers = 1
    };

    if(vkCreateFramebuffer(_device, &createInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create framebuffer !");
    }
}

VkFramebufferHandler::~VkFramebufferHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroyFramebuffer(_device, _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkFramebufferHandler Destroyed !");
#endif
}

VkFramebufferHandler::VkFramebufferHandler(VkFramebufferHandler && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkFramebufferHandler Move Creation !");
#endif

    _internal = other._internal;
    _device = other._device;
    other._internal = VK_NULL_HANDLE;
}

VkFramebufferHandler& VkFramebufferHandler::operator=(VkFramebufferHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        _device = other._device;
        other._internal = VK_NULL_HANDLE;
    }

    return *this;
}
