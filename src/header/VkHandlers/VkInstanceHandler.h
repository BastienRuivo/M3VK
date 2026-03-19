#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

class VkInstanceHandler
{
    public:
    VkInstanceHandler();
    ~VkInstanceHandler();

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    VkInstanceHandler(VkInstanceHandler&& other) noexcept;
    VkInstanceHandler& operator=(VkInstanceHandler&& other) noexcept;

    VkInstanceHandler(const VkInstanceHandler&) = delete;
    VkInstanceHandler& operator=(const VkInstanceHandler&) = delete;

    inline VkInstance Get() const
    {
        return _internal;
    }

    private:
    VkInstance _internal = VK_NULL_HANDLE;
    std::vector<const char*> GetRequiredExtensions() const;
};
