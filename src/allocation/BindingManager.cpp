#include "allocation/BindingManager.h"
#include "application/ApplicationInfo.h"
#include "allocation/DescriptorPool.h"
#include <cstdint>
#include <vulkan/vulkan.hpp>

#include "ShaderBindings.h"
#include "rendering/CommandBuffer.h"
#include "rendering/ImageHelper.h"
const vk::DescriptorBindingFlags bindlessFlags =
    vk::DescriptorBindingFlagBits::eUpdateAfterBind |
    vk::DescriptorBindingFlagBits::ePartiallyBound;
BindingManager::BindingManager()
:
_bindlessPool(DescriptorPool::Builder()
    .SetMaxSets(1)
    .SetFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind)
    .AddLayout(DescriptorPool::LayoutBuilder()
    .AddBinding(BINDING_TEXTURES, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute, bindlessFlags, ApplicationInfo::Constant::MaxBindlessTextureCount)
    .AddBinding(BINDING_CAMERA_BUFFER, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute, bindlessFlags, RessourceUsage::PerFrame)
    .AddBinding(BINDING_MATERIAL_BUFFER, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, {}, RessourceUsage::Static)
    .AddBinding(BINDING_INSTANCE_DATA_BUFFER, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute, {}, RessourceUsage::Static)
    .AddBinding(BINDING_CLEAR_DRAW_INDIRECT_BUFFER, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, {}, RessourceUsage::Static)
    .AddBinding(BINDING_VISIBLE_DRAW_INDIRECT_BUFFER, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, bindlessFlags, RessourceUsage::PerFrame)
    .AddBinding(BINDING_VISIBLE_INSTANCE_INDIRECTION_BUFFER, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute, bindlessFlags, RessourceUsage::PerFrame)
    .AddBinding(BINDING_SKYBOX_TEXTURE, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, {}, RessourceUsage::Static)
    .AddBinding(BINDING_HIZ_TEXTURE, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute, bindlessFlags, RessourceUsage::PerFrame, ApplicationInfo::Constant::MaxMipCount)
    .SetFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool)
    )
    .Build()),
_globalPushConstantRanges({
    vk::PushConstantRange{}
        .setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute)
        .setOffset(0)
        .setSize(COMMON_INDEXES_OFFSET + ShaderConstantsSize)
}),
_globalLayout(GlobalSetLayouts(), _globalPushConstantRanges)
{
    uint32_t staticCounts[] =
    {
        1
    };
    _globalSet = AllocateBindlessInternal({staticCounts}, _bindlessPool.Layout(0));
}

DescriptorSetHandle BindingManager::AllocateBindlessInternal(std::span<uint32_t> counts, vk::DescriptorSetLayout layout)
{
    vk::DescriptorSetVariableDescriptorCountAllocateInfo countInfo = vk::DescriptorSetVariableDescriptorCountAllocateInfo{}
        .setDescriptorCounts(counts);            // must match DescriptorSetAllocateInfo::descriptorSetCount

    vk::DescriptorSetAllocateInfo allocateInfo = vk::DescriptorSetAllocateInfo{}
        .setPNext(&countInfo)
        .setDescriptorPool(_bindlessPool.Pool())
        .setSetLayouts(layout);

    vk::DescriptorSet descriptorSet;
    if(ApplicationInfo::Device().allocateDescriptorSets(&allocateInfo, &descriptorSet) != vk::Result::eSuccess)
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

void BindingManager::UpdateBindlessTextureInternal(uint32_t index, const ImageReference& img, vk::Sampler sampler)
{
    vk::Format format = img.Format;
    bool isDepthFormat = format == vk::Format::eD32Sfloat || format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint || format == vk::Format::eD16Unorm;
    bool hasStencil = format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint || format == vk::Format::eD16UnormS8Uint;
    vk::DescriptorImageInfo imageInfo = vk::DescriptorImageInfo{}
        .setSampler(sampler)
        .setImageView(img.View)
        .setImageLayout(isDepthFormat ? (hasStencil ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eDepthReadOnlyOptimal) : vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::WriteDescriptorSet write = vk::WriteDescriptorSet{}
        .setDstSet(_globalSet.Set)
        .setDstBinding(0)
        .setDstArrayElement(index)
        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
        .setImageInfo(imageInfo);

    DescriptorHelper::UpdateDescriptorSet(std::span(&write, 1), {});
}

uint32_t BindingManager::RegisterBindlessTextureInternal(GPUImage&& texture, vk::Sampler sampler)
{
    ImageReference ref = texture.Internal();

    uint32_t index = _texturePool.Register(std::move(texture));

    UpdateBindlessTextureInternal(index, ref, sampler);

    return index;
}

void BindingManager::RegisterBuffer(const vk::DescriptorBufferInfo& info, vk::DescriptorType type, uint32_t dstBinding, uint32_t dstArrayElement)
{
    vk::DescriptorSet set = _globalSet.Set;

    vk::WriteDescriptorSet write = vk::WriteDescriptorSet{}
        .setDstSet(set)
        .setDstBinding(dstBinding)
        .setDstArrayElement(dstArrayElement)
        .setDescriptorType(type)
        .setBufferInfo(info);

    DescriptorHelper::UpdateDescriptorSet(std::span(&write, 1), {});
}

void BindingManager::RegisterImage(const vk::DescriptorImageInfo& info, uint32_t dstBinding, uint32_t dstArrayElement) const
{
    vk::DescriptorSet set = _globalSet.Set;

    vk::WriteDescriptorSet write = vk::WriteDescriptorSet{}
        .setDstSet(set)
        .setDstBinding(dstBinding)
        .setDstArrayElement(dstArrayElement)
        .setDescriptorType(vk::DescriptorType::eStorageImage)
        .setImageInfo(info);

    DescriptorHelper::UpdateDescriptorSet(std::span(&write, 1), {});
}

void BindingManager::RegisterTexture(const vk::DescriptorImageInfo& info, uint32_t dstBinding, uint32_t dstArrayElement) const
{
    vk::DescriptorSet set = _globalSet.Set;

    vk::WriteDescriptorSet write = vk::WriteDescriptorSet{}
        .setDstSet(set)
        .setDstBinding(dstBinding)
        .setDstArrayElement(dstArrayElement)
        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
        .setImageInfo(info);

    DescriptorHelper::UpdateDescriptorSet(std::span(&write, 1), {});
}

BindingManager::~BindingManager()
{
}

void BindingManager::BindlessTexture::Dispose(BindingManager& allocator)
{
    for (size_t i = 0; i < RessourceUsageCount(Usage); i++)
    {
        allocator._texturePool.Remove(Index[i]);
    }
}

void BindingManager::BindlessTexture::Bind(const BindingManager& allocator, bool asImage, uint32_t binding, vk::Sampler sampler) const
{
    uint32_t count  = RessourceUsageCount(Usage);
    for(int i = 0; i < count; i++)
    {
        const auto & tex = Texture(allocator).Internal();
        if(asImage)
        {
            allocator.RegisterImage(ImageHelper::ImageBinding(tex, sampler, vk::ImageLayout::eGeneral).Descriptor, binding, i);
        }
        else
        {
            allocator.RegisterTexture(ImageHelper::ImageBinding(tex, sampler).Descriptor, binding, i);
        }
    }
}

bool BindingManager::BindlessTexture::Resize(BindingManager& allocator, uint32_t width, uint32_t height)
{
    uint32_t count = RessourceUsageCount(Usage);
    bool hasResize = false;
    for (size_t i = 0; i < count; i++)
    {
        GPUImage& image = allocator._texturePool.Texture(Index[i]);
        hasResize |= image.Resize(width, height);
        allocator.UpdateBindlessTextureInternal(Index[i], image.Internal(), Sampler);
    }
    return hasResize;
}

void BindingManager::BindlessTexture::TransistionAllLayoutCommand(const BindingManager& allocator, const CommandBuffer& cmdBuffer, vk::ImageLayout layout)
{
    uint32_t count = RessourceUsageCount(Usage);
    for (size_t i = 0; i < count; i++)
    {
        const GPUImage& image = allocator._texturePool.Texture(Index[i]);
        ImageHelper::TransitionLayoutCommand(cmdBuffer, image.Internal(), vk::ImageLayout::eUndefined, layout);
    }
}
