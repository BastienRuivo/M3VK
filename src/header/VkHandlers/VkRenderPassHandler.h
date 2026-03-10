#pragma once

#include <vulkan/vulkan_core.h>

class VkRenderPassHandler
{
    public:
    VkRenderPassHandler(VkDevice device, VkFormat imageFormat);
    ~VkRenderPassHandler();

    VkRenderPass Get() const;

    public:

    private:
    VkRenderPass _internal = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
};
