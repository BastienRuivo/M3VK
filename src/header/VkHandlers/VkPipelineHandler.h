#pragma once

#include <vulkan/vulkan_core.h>

class VkPipelineHandler
{
    public:
    VkPipelineHandler(const VkExtent2D& appExtent, VkDevice device, VkSampleCountFlagBits msaaSampleCount, VkPipelineLayout pipelineLayout, VkFormat swapChainFormat, VkFormat depthFormat);
    ~VkPipelineHandler();

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    VkPipelineHandler(VkPipelineHandler&& other) noexcept;
    VkPipelineHandler& operator=(VkPipelineHandler&& other) noexcept;

    VkPipelineHandler(const VkPipelineHandler&) = delete;
    VkPipelineHandler& operator=(const VkPipelineHandler&) = delete;

    inline VkPipeline Get() const
    {
        return _internal;
    }

    private:
    VkPipeline _internal = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
};
