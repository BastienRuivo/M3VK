#pragma once

#include "handler/Handlers.h"
#include <cstdint>
#include <vulkan/vulkan.hpp>

class VkImageViewHandler : public Handler<vk::ImageView>
{
    public:
    VkImageViewHandler() {};
    VkImageViewHandler(vk::Image image, vk::Format format, uint32_t mipCount, vk::ImageAspectFlags aspectMask, vk::ImageViewType type);
    VkImageViewHandler(vk::Image image, vk::Format format, uint32_t mipCount, vk::ImageAspectFlags aspectMask);
    VkImageViewHandler(vk::Image image, vk::Format format, uint32_t mipCount);
    ~VkImageViewHandler();

    VkImageViewHandler(VkImageViewHandler&& other) noexcept;
    VkImageViewHandler& operator=(VkImageViewHandler&& other) noexcept;

    inline vk::Format Format() const
    {
        return _format;
    }

    private:
    vk::Format _format = vk::Format::eUndefined;
};
