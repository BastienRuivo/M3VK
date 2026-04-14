#pragma once

#include <initializer_list>
#include <vulkan/vulkan_core.h>

class VkPipelineLayoutHandler
{
    public:
    VkPipelineLayoutHandler(std::initializer_list<VkDescriptorSetLayout> descriptorLayouts, std::initializer_list<VkPushConstantRange> pushConstantRanges);
    VkPipelineLayoutHandler(const VkDescriptorSetLayout* descriptorLayouts, uint32_t descriptorLayoutCount, const VkPushConstantRange* pushConstantRanges, uint32_t pushConstantRangeCount);
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
};
