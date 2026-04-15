#pragma once

#include "header/GPUImage.h"
#include <vector>

struct Material
{
    public:
    ImageHelper::ImageBinding AlbedoMap;
    VkDescriptorSet DescriptorSet;

    Material( const ImageHelper::ImageBinding& albedoMap, VkDescriptorSet descriptorSet ) : AlbedoMap(albedoMap), DescriptorSet(descriptorSet) {}

    static std::vector<VkDescriptorSetLayoutBinding> GetBindings()
    {
        return
        {
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr
            }
        };
    }

    static std::vector<VkWriteDescriptorSet> GetDescriptorWrites( const Material& material )
    {
        return
        {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = material.DescriptorSet,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &material.AlbedoMap.Descriptor,
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr
            }
        };
    }
};
