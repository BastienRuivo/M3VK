#include "header/VkHandlers/VkFramebufferHandler.h"
#include "header/DebugLayer.h"
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

VkFramebufferHandler::VkFramebufferHandler(VkDevice device, VkRenderPass renderPass, VkExtent2D imageExtent, std::vector<VkImageView> imageViews)
{
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkFramebufferHandler Creation !");

    _device = device;

    VkFramebufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = imageViews.size();
    createInfo.pAttachments = imageViews.data();
    createInfo.width = imageExtent.width;
    createInfo.height = imageExtent.height;
    createInfo.layers = 1;

    if(vkCreateFramebuffer(_device, &createInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create framebuffer !");
    }
}

VkFramebufferHandler::VkFramebufferHandler(VkFramebufferHandler&& other)
{
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkFramebufferHandler Copy Creation !");
    _device = other._device;
    _internal = other._internal;
    other._internal = VK_NULL_HANDLE;
}

VkFramebuffer VkFramebufferHandler::Get() const
{
    return _internal;
}

VkFramebufferHandler::~VkFramebufferHandler()
{
    if(_internal != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(_device, _internal, nullptr);
    }
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkFramebufferHandler Destroyed !");
}
