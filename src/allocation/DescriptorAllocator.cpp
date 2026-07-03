#include "allocation/DescriptorAllocator.h"
#include "application/ApplicationInfo.h"
#include "allocation/DescriptorPool.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

#include "ShaderBindings.h"

DescriptorAllocator::DescriptorAllocator()
:
_globalSetLayouts({
    DescriptorPool::LayoutBuilder()
    .AddBinding(BINDING_TEXTURES, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, 0, ApplicationInfo::Constant::MaxBindlessTextureCount)
    .AddBinding(BINDING_CAMERA_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, 0, RessourceUsage::PerFrame)
    .AddBinding(STATIC_BINDING_MATERIAL_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, RessourceUsage::Static)
    .AddBinding(STATIC_BINDING_INSTANCE_DATA_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT, 0, RessourceUsage::Static)
    .AddBinding(STATIC_BINDING_CLEAR_DRAW_INDIRECT_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0, RessourceUsage::Static)
    .AddBinding(BINDING_VISIBLE_DRAW_INDIRECT_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0, RessourceUsage::PerFrame)
    .AddBinding(BINDING_VISIBLE_INSTANCE_INDIRECTION_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT, 0, RessourceUsage::PerFrame)
    .AddBinding(STATIC_BINDING_SKYBOX_TEXTURE, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, RessourceUsage::Static)
    .Build(),
}),
_globalPushConstantRanges({
    VkPushConstantRange
    {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(CommonIndexes) + ShaderConstantsSize,
    }
}),
_globalLayout(_globalSetLayouts, _globalPushConstantRanges),
_bindlessPool(DescriptorPool::Builder()
    .SetMaxSets(1)
    .SetFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT)
    .AddLayout(DescriptorPool::LayoutBuilder()
        .AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
            | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT, ApplicationInfo::Constant::MaxBindlessTextureCount + ApplicationInfo::Constant::MaxOtherTextureCount)
        .AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
            | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT, ApplicationInfo::Constant::MaxBufferCount)
        .SetFlags(VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT)
    )
    .Build())
{
    uint32_t staticCounts[] =
    {
        ApplicationInfo::Constant::MaxBufferCount
    };
    _globalSet = AllocateBindlessInternal({staticCounts}, _globalSetLayouts[0]);
}

DescriptorSetHandle DescriptorAllocator::AllocateBindlessInternal(std::span<uint32_t> counts, VkDescriptorSetLayout layout)
{
    VkDescriptorSetVariableDescriptorCountAllocateInfo countInfo{
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
        .descriptorSetCount = static_cast<uint32_t>(counts.size()),             // must match VkDescriptorSetAllocateInfo::descriptorSetCount
        .pDescriptorCounts  = counts.data()
    };

    VkDescriptorSetAllocateInfo allocateInfo
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = &countInfo,
        .descriptorPool = _bindlessPool.Pool(),
        .descriptorSetCount = countInfo.descriptorSetCount,
        .pSetLayouts = &layout
    };

    VkDescriptorSet descriptorSet;
    if(vkAllocateDescriptorSets(ApplicationInfo::Device(), &allocateInfo, &descriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    return
    {
        .Set = descriptorSet,
        .Layout =layout,
        .Pool = _bindlessPool.Pool()
    };
}

uint32_t DescriptorAllocator::RegisterBindlessTextureInternal(GPUImage&& texture, VkSampler sampler)
{
    VkFormat format = texture.Internal().Format;
    bool isDepthFormat = format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D16_UNORM;
    bool hasStencil = format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D16_UNORM_S8_UINT;
    VkDescriptorImageInfo imageInfo
    {
        .sampler = sampler,
        .imageView = texture.View(),
        .imageLayout = isDepthFormat ? (hasStencil ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL) : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    uint32_t index = _texturePool.Register(std::move(texture));

    VkDescriptorSet set = _globalSet.Set;

    VkWriteDescriptorSet write =
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = set,
        .dstBinding = 0,
        .dstArrayElement = index,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &imageInfo,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr
    };

    DescriptorHelper::UpdateDescriptorSet(std::span(&write, 1), {});

    return index;
}

void DescriptorAllocator::RegisterBuffer(const VkDescriptorBufferInfo& info, VkDescriptorType type, uint32_t dstBinding, uint32_t dstArrayElement)
{
    VkDescriptorSet set = _globalSet.Set;

    VkWriteDescriptorSet write =
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = set,
        .dstBinding = dstBinding,
        .dstArrayElement = dstArrayElement,
        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = nullptr,
        .pBufferInfo = &info,
        .pTexelBufferView = nullptr
    };

    DescriptorHelper::UpdateDescriptorSet(std::span(&write, 1), {});
}

void DescriptorAllocator::RegisterTexture(const VkDescriptorImageInfo& info, uint32_t dstBinding, uint32_t dstArrayElement)
{
    VkDescriptorSet set = _globalSet.Set;

    VkWriteDescriptorSet write =
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = set,
        .dstBinding = dstBinding,
        .dstArrayElement = dstArrayElement,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr
    };

    DescriptorHelper::UpdateDescriptorSet(std::span(&write, 1), {});
}

DescriptorAllocator::~DescriptorAllocator()
{
    for(uint32_t i = 0; i < _globalSetLayouts.size(); i++)
    {
        vkDestroyDescriptorSetLayout(ApplicationInfo::Device(), _globalSetLayouts[i], nullptr);
    }
}

void DescriptorAllocator::RemoveTexture(BindlessTextureHandle handle)
{
    for (size_t i = 0; i < RessourceUsageCount(handle.usage); i++)
    {
        _texturePool.Remove(handle.index[i]);
    }
}
