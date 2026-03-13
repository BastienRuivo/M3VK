#pragma once

#include <vulkan/vulkan_core.h>

class VkRenderPassHandler
{
    public:
    VkRenderPassHandler(VkDevice device, VkFormat imageFormat);
    ~VkRenderPassHandler();

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    VkRenderPassHandler(VkRenderPassHandler&& other) noexcept;
    VkRenderPassHandler& operator=(VkRenderPassHandler&& other) noexcept;

    VkRenderPassHandler(const VkRenderPassHandler&) = delete;
    VkRenderPassHandler& operator=(const VkRenderPassHandler&) = delete;

    VkRenderPass Get() const;

    public:

    private:
    VkRenderPass _internal = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
};
