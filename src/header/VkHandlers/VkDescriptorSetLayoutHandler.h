#pragma once

#include <vulkan/vulkan_core.h>

class VkDescriptorSetLayoutHandler
{
    public:
    VkDescriptorSetLayoutHandler(VkDevice device);
    ~VkDescriptorSetLayoutHandler();

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    VkDescriptorSetLayoutHandler(VkDescriptorSetLayoutHandler&& other) noexcept;
    VkDescriptorSetLayoutHandler& operator=(VkDescriptorSetLayoutHandler&& other) noexcept;

    VkDescriptorSetLayoutHandler(const VkDescriptorSetLayoutHandler&) = delete;
    VkDescriptorSetLayoutHandler& operator=(const VkDescriptorSetLayoutHandler&) = delete;

    inline VkDescriptorSetLayout Get() const
    {
        return _internal;
    }

    private:
    VkDescriptorSetLayout _internal = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
};
