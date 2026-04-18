#pragma once

#include "header/ApplicationInfo.h"
#include "header/DescriptorPool.h"
#include "header/GPUImage.h"
#include <memory>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

struct Material
{
    public:
    ImageHelper::ImageBinding AlbedoMap;
    std::shared_ptr<VkDescriptorSet> DescriptorSet;

    Material( const ImageHelper::ImageBinding& albedoMap, const DescriptorPool& materialPool )
    : AlbedoMap(albedoMap)
    {
        DescriptorSet = std::shared_ptr<VkDescriptorSet>(
            new VkDescriptorSet(materialPool.Allocate()),
            [pool = materialPool.Pool()](VkDescriptorSet* descriptorSet)
            {
                vkFreeDescriptorSets(ApplicationInfo::Device(), pool, 1, descriptorSet);
                delete descriptorSet;
            }
        );
        materialPool.UpdateDescriptorSet(Material::GetDescriptorWrites(*this), {});
    }

    Material( const Material& other ) : AlbedoMap(other.AlbedoMap), DescriptorSet(other.DescriptorSet) {}
    Material( Material&& other ) noexcept : AlbedoMap(std::move(other.AlbedoMap)), DescriptorSet(std::move(other.DescriptorSet)) {}

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
                .dstSet = *material.DescriptorSet,
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
