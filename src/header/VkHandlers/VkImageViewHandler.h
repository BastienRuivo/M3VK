#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>

class VkImageViewHandler
{
    public:
    VkImageViewHandler() {};
    VkImageViewHandler(VkDevice device, VkImage image, VkFormat format, uint32_t mipCount, VkImageAspectFlags aspectMask);
    VkImageViewHandler(VkDevice device, VkImage image, VkFormat format, uint32_t mipCount);
    ~VkImageViewHandler();

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    VkImageViewHandler(VkImageViewHandler&& other) noexcept;
    VkImageViewHandler& operator=(VkImageViewHandler&& other) noexcept;

    VkImageViewHandler(const VkImageViewHandler&) = delete;
    VkImageViewHandler& operator=(const VkImageViewHandler&) = delete;

    inline VkImageView Get() const
    {
        return _internal;
    }

    inline VkFormat GetFormat() const
    {
        return _format;
    }

    private:
    VkImageView _internal = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    VkFormat _format = VK_FORMAT_UNDEFINED;
};
