#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

class VkFramebufferHandler
{
    public:
    VkFramebufferHandler(VkDevice device, VkRenderPass renderPass, VkExtent2D imageExtent, std::vector<VkImageView> imageViews);

    ~VkFramebufferHandler();

    VkFramebuffer Get() const;

    public:

    private:
    VkFramebuffer _internal = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
};
