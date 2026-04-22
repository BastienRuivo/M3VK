#pragma once

#include "rendering/GPUImage.h"

namespace ImageHelper
{
    struct ImageBinding
    {
        ImageReference Image;
        VkDescriptorImageInfo Descriptor;
        VkSampler Sampler = VK_NULL_HANDLE;

        ImageBinding(const ImageReference& image, VkSampler sampler) : Image(image), Sampler(sampler)
        {
            Descriptor =
            {
                .sampler = sampler,
                .imageView = image.View,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
        }
        ImageBinding() = default;
    };

    void TransitionLayoutCommand(const CommandBuffer& cmdBuffer, const ImageReference& image, VkImageLayout oldLayout, VkImageLayout newLayout);
}
