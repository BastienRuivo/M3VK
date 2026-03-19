#pragma once

#include <vulkan/vulkan_core.h>

class VkPipelineLayoutHandler
{
    public:
    VkPipelineLayoutHandler(VkDevice device, VkDescriptorSetLayout descriptorLayout);
    ~VkPipelineLayoutHandler();

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    VkPipelineLayoutHandler(VkPipelineLayoutHandler&& other) noexcept;
    VkPipelineLayoutHandler& operator=(VkPipelineLayoutHandler&& other) noexcept;

    VkPipelineLayoutHandler(const VkPipelineLayoutHandler&) = delete;
    VkPipelineLayoutHandler& operator=(const VkPipelineLayoutHandler&) = delete;

    inline VkPipelineLayout Get() const
    {
        return _internal;
    }

    public:

    private:
    VkPipelineLayout _internal = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
};
