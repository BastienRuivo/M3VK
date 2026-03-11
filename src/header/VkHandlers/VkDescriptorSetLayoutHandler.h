#pragma once

#include <vulkan/vulkan_core.h>

class VkDescriptorSetLayoutHandler
{
    public:
    VkDescriptorSetLayoutHandler(VkDevice device);
    ~VkDescriptorSetLayoutHandler();

    VkDescriptorSetLayout Get() const;

    public:

    private:
    VkDescriptorSetLayout _internal = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
};
