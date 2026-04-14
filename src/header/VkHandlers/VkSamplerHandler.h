#pragma once

#include <vulkan/vulkan_core.h>

class VkSamplerHandler
{
    public:
    VkSamplerHandler() {};
    VkSamplerHandler(VkDevice device);
    ~VkSamplerHandler();

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    VkSamplerHandler(VkSamplerHandler&& other) noexcept;
    VkSamplerHandler& operator=(VkSamplerHandler&& other) noexcept;

    VkSamplerHandler(const VkSamplerHandler&) = delete;
    VkSamplerHandler& operator=(const VkSamplerHandler&) = delete;

    inline VkSampler Get() const
    {
        return _internal;
    }

    private:
    VkSampler _internal = VK_NULL_HANDLE;
};
