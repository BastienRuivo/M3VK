#include "handler/VkImageViewHandler.h"
#include "application/ApplicationInfo.h"
#include "application/ApplicationHelper.h"
#include <stdexcept>
#include <vulkan/vulkan.hpp>

VkImageViewHandler::VkImageViewHandler(vk::Image image, vk::Format format, uint32_t mipCount) :
    VkImageViewHandler(image, format, mipCount, ApplicationHelper::GetImageAspectFlags(format)) {}

VkImageViewHandler::VkImageViewHandler(vk::Image image, vk::Format format, uint32_t mipCount, vk::ImageAspectFlags aspectMask) :
    VkImageViewHandler(image, format, mipCount, aspectMask, vk::ImageViewType::e2D) {}

VkImageViewHandler::VkImageViewHandler(vk::Image image, vk::Format format, uint32_t mipCount, vk::ImageAspectFlags aspectMask, vk::ImageViewType type)
{
    _format = format;

    vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange{}
        .setAspectMask(aspectMask)
        .setBaseMipLevel(0)
        .setLevelCount(mipCount)
        .setBaseArrayLayer(0)
        .setLayerCount(type == vk::ImageViewType::eCube ? 6u : 1u);

    vk::ImageViewCreateInfo createInfo = vk::ImageViewCreateInfo{}
        .setImage(image)
        .setViewType(type)
        .setFormat(format)
        .setSubresourceRange(subresourceRange);

    if(ApplicationInfo::Device().createImageView(&createInfo, nullptr, &_internal) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create swap chain images !");
    }
}

VkImageViewHandler::~VkImageViewHandler()
{
    if(!_internal) return;

    ApplicationInfo::Device().destroyImageView(_internal);
}

VkImageViewHandler::VkImageViewHandler(VkImageViewHandler&& other) noexcept
{
    _internal = std::exchange(other._internal, VK_NULL_HANDLE);
    _format = std::exchange(other._format, vk::Format::eUndefined);
}

VkImageViewHandler& VkImageViewHandler::operator=(VkImageViewHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = std::exchange(other._internal, VK_NULL_HANDLE);
        _format = std::exchange(other._format, vk::Format::eUndefined);
    }
    return *this;
}
