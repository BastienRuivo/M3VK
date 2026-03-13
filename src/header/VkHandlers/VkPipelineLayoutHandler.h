#pragma once

#include <vulkan/vulkan_core.h>

class VkPipelineLayoutHandler
{
    public:
    VkPipelineLayoutHandler(VkDevice device, VkDescriptorSetLayout descriptorLayout);
    ~VkPipelineLayoutHandler();

    VkPipelineLayout Get() const;

    public:

    private:
    VkPipelineLayout _internal = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
};
