#pragma once

#include <vulkan/vulkan_core.h>

class VkFenceHandler
{
    public:
    VkFenceHandler();
    ~VkFenceHandler();

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    VkFenceHandler(VkFenceHandler&& other) noexcept;
    VkFenceHandler& operator=(VkFenceHandler&& other) noexcept;

    VkFenceHandler(const VkFenceHandler&) = delete;
    VkFenceHandler& operator=(const VkFenceHandler&) = delete;

    void Wait(uint64_t timeout) const;
    void Reset() const;

    inline VkFence Get() const
    {
        return _internal;
    }

    private:
    VkFence _internal = VK_NULL_HANDLE;
};
