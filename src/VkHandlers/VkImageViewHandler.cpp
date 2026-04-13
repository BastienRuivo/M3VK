#include "header/VkHandlers/VkImageViewHandler.h"
#include "header/ProjectHelper.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

VkImageViewHandler::VkImageViewHandler(VkDevice device, VkImage image, VkFormat format, uint32_t mipCount) :
    VkImageViewHandler(device, image, format, mipCount, ProjectHelper::GetImageAspectFlags(format)) {}

VkImageViewHandler::VkImageViewHandler(VkDevice device, VkImage image, VkFormat format, uint32_t mipCount, VkImageAspectFlags aspectMask)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkImageViewHandler Creation !");
#endif

    _device = device;

    VkImageViewCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange =
        {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = mipCount,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    if(vkCreateImageView(_device, &createInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create swap chain images !");
    }
}
VkImageViewHandler::~VkImageViewHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroyImageView(_device, _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkImageViewHandler Destroyed !");
#endif
}

VkImageViewHandler::VkImageViewHandler(VkImageViewHandler && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkImageViewHandler Move Creation !");
#endif

    _internal = other._internal;
    _device = other._device;
    _format = other._format;
    other._internal = VK_NULL_HANDLE;
    other._device = VK_NULL_HANDLE;
    other._format = VK_FORMAT_UNDEFINED;
}

VkImageViewHandler& VkImageViewHandler::operator=(VkImageViewHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        _device = other._device;
        _format = other._format;
        other._internal = VK_NULL_HANDLE;
        other._device = VK_NULL_HANDLE;
        other._format = VK_FORMAT_UNDEFINED;
    }

    return *this;
}
