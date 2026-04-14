#pragma once

#include <vulkan/vulkan_core.h>

class VkSemaphoreHandler
{
    public:
    VkSemaphoreHandler();
    ~VkSemaphoreHandler();

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    VkSemaphoreHandler(VkSemaphoreHandler&& other) noexcept;
    VkSemaphoreHandler& operator=(VkSemaphoreHandler&& other) noexcept;

    VkSemaphoreHandler(const VkSemaphoreHandler&) = delete;
    VkSemaphoreHandler& operator=(const VkSemaphoreHandler&) = delete;

    inline VkSemaphore Get() const
    {
        return _internal;
    }

    private:
    VkSemaphore _internal = VK_NULL_HANDLE;
};
