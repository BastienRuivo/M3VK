#pragma once

#include <vulkan/vulkan_core.h>

class VkCommandPoolHandler
{
    public:
    VkCommandPoolHandler(VkDevice device);
    ~VkCommandPoolHandler();

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    VkCommandPoolHandler(VkCommandPoolHandler&& other) noexcept;
    VkCommandPoolHandler& operator=(VkCommandPoolHandler&& other) noexcept;

    VkCommandPoolHandler(const VkCommandPoolHandler&) = delete;
    VkCommandPoolHandler& operator=(const VkCommandPoolHandler&) = delete;

    inline VkCommandPool Get() const
    {
        return _internal;
    }

    private:
    VkCommandPool _internal = VK_NULL_HANDLE;
};
