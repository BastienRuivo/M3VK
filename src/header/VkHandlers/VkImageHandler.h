#pragma once

#include <vulkan/vulkan_core.h>

class VkImageHandler
{
    public:
    VkImageHandler(VkDevice device, VkImage image, VkFormat format);
    ~VkImageHandler();

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    VkImageHandler(VkImageHandler&& other) noexcept;
    VkImageHandler& operator=(VkImageHandler&& other) noexcept;

    VkImageHandler(const VkImageHandler&) = delete;
    VkImageHandler& operator=(const VkImageHandler&) = delete;

    inline VkImage Get() const
    {
        return _internal;
    }

    inline VkFormat GetFormat() const
    {
        return _format;
    }

    private:
    VkImage _internal = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    VkFormat _format = VK_FORMAT_UNDEFINED;
};
