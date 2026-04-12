#pragma once

#include <cstdint>
#include <initializer_list>
#include <vulkan/vulkan_core.h>

class VkDescriptorPoolHandler
{
    public:

    VkDescriptorPoolHandler(VkDevice device, std::initializer_list<VkDescriptorPoolSize> poolSizes, uint32_t maxSets);
    VkDescriptorPoolHandler(VkDevice device, const VkDescriptorPoolSize* poolSizes, uint32_t poolSizesCount, uint32_t maxSets);
    ~VkDescriptorPoolHandler();

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    VkDescriptorPoolHandler(VkDescriptorPoolHandler&& other) noexcept;
    VkDescriptorPoolHandler& operator=(VkDescriptorPoolHandler&& other) noexcept;

    VkDescriptorPoolHandler(const VkDescriptorPoolHandler&) = delete;
    VkDescriptorPoolHandler& operator=(const VkDescriptorPoolHandler&) = delete;

    inline VkDescriptorPool Get() const
    {
        return _internal;
    }

    private:
    VkDescriptorPool _internal = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
};
