#pragma once

#include "header/DescriptorPool.h"
#include "header/GPUImage.h"
#include "header/GraphicsBuffer.h"
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

struct Material
{
    public:
    ImageHelper::ImageBinding AlbedoMap;
    BufferHelper::BufferBinding PropertyBuffer;
    std::shared_ptr<VkDescriptorSet> DescriptorSet;

    Material( const ImageHelper::ImageBinding& albedoMap, const BufferHelper::BufferBinding& propertyBuffer,
        const DescriptorPool& materialPool )
    : AlbedoMap(albedoMap), PropertyBuffer(propertyBuffer)
    {
        AlbedoMap = albedoMap;
        DescriptorSet = std::shared_ptr<VkDescriptorSet>(
            new VkDescriptorSet(materialPool.Allocate()),
            [&pool = materialPool](VkDescriptorSet* descriptorSet)
            {
                pool.Free(*descriptorSet);
                delete descriptorSet;
            }
        );
        materialPool.UpdateDescriptorSet(Material::GetDescriptorWrites(*this), {});
    }

    Material( const Material& other ) : AlbedoMap(other.AlbedoMap), DescriptorSet(other.DescriptorSet), PropertyBuffer(other.PropertyBuffer) {}
    Material( Material&& other ) noexcept : AlbedoMap(std::move(other.AlbedoMap)), DescriptorSet(std::move(other.DescriptorSet)), PropertyBuffer(std::move(other.PropertyBuffer)) {}

    static std::vector<VkDescriptorSetLayoutBinding> GetBindings()
    {
        uint32_t binding = 0;
        return
        {{
                .binding = binding++,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr
            },
            {
                .binding = binding++,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr
            }
        };
    }

    static std::vector<VkWriteDescriptorSet> GetDescriptorWrites( const Material& material)
    {
        uint32_t binding = 0;
        return
        {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = *material.DescriptorSet,
                .dstBinding = binding++,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = material.PropertyBuffer.DescriptorType,
                .pImageInfo = nullptr,
                .pBufferInfo = &material.PropertyBuffer.Descriptor,
                .pTexelBufferView = nullptr
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = *material.DescriptorSet,
                .dstBinding = binding++,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &material.AlbedoMap.Descriptor,
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr
            },
        };
    }
};
