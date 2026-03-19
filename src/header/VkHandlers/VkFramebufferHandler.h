#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

class VkFramebufferHandler
{
    public:
    VkFramebufferHandler(VkDevice device, VkRenderPass renderPass, VkExtent2D imageExtent, std::vector<VkImageView> imageViews);

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    VkFramebufferHandler(VkFramebufferHandler&& other) noexcept;
    VkFramebufferHandler& operator=(VkFramebufferHandler&& other) noexcept;

    VkFramebufferHandler(const VkFramebufferHandler&) = delete;
    VkFramebufferHandler& operator=(const VkFramebufferHandler&) = delete;

    ~VkFramebufferHandler();

    inline VkFramebuffer Get() const
    {
        return _internal;
    }

    public:

    private:
    VkFramebuffer _internal = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
};
