#pragma once

#include <vulkan/vulkan_core.h>

class VkPipelineHandler
{
    public:
    VkPipelineHandler(const VkExtent2D& appExtent, VkDevice device, VkPipelineLayout pipelineLayout, VkRenderPass renderPass);
    ~VkPipelineHandler();

    VkPipeline Get() const;

    private:
    VkPipeline _internal = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
};
