#pragma once

#include "handler/Handlers.h"
#include "allocation/BindlessTexturePool.h"
#include "allocation/DescriptorPool.h"
#include "rendering/GPUImage.h"
#include <cassert>
#include <cstdint>
#include <vulkan/vulkan_core.h>

class DescriptorAllocator
{
    public:
    struct BindlessTextureHandle
    {
        RessourceUsage usage;
        std::array<uint32_t, ApplicationInfo::Constant::MaxFrameInFlight> index;
        uint32_t StaticIndex() const { assert(usage == RessourceUsage::Static); return index[0]; }
    };

    DescriptorAllocator();
    ~DescriptorAllocator();

    static constexpr uint32_t ShaderConstantsSize = 64;

    template<typename... Args>
    BindlessTextureHandle RegisterBindlessTexture(RessourceUsage usage, VkSampler sampler, Args&&... args)
    {
        BindlessTextureHandle handle
        {
            .usage = usage,
            .index = {UINT32_MAX, UINT32_MAX}
        };
        uint32_t count = RessourceUsageCount(usage);
        for (size_t i = 0; i < count; i++)
        {
            GPUImage image = GPUImage(std::forward<Args>(args)...);
            handle.index[i] = RegisterBindlessTextureInternal(std::move(image), sampler);
        }
        return handle;
    }

    void RegisterBuffer(const VkDescriptorBufferInfo& info, VkDescriptorType type, uint32_t dstBinding, uint32_t dstArrayElement);
    void RegisterTexture(const VkDescriptorImageInfo& info, uint32_t dstBinding, uint32_t dstArrayElement);
    void RemoveTexture(BindlessTextureHandle handle);

    inline DescriptorSetHandle GlobalDescriptorSet() const { return _globalSet; }
    inline VkPipelineLayout GlobalLayout() const { return _globalLayout.Internal(); }
    inline const std::vector<VkPushConstantRange>& GlobalPushConstantRanges() const { return _globalPushConstantRanges; }
    inline const std::vector<VkDescriptorSetLayout>& GlobalSetLayouts() const { return _globalSetLayouts; }
    inline BindlessTexturePool& TexturePool() { return _texturePool; }
    inline GPUImage& Texture(BindlessTextureHandle handle) { return _texturePool.Texture(handle.index[RessourceUsageToCurrentIndex(handle.usage)]); }
    inline GPUImage& PreviousTexture(BindlessTextureHandle handle) { return _texturePool.Texture(handle.index[RessourceUsageToPreviousIndex(handle.usage)]); }

    private:
    DescriptorSetHandle AllocateBindlessInternal(std::span<uint32_t> counts, VkDescriptorSetLayout layout);
    uint32_t RegisterBindlessTextureInternal(GPUImage&& texture, VkSampler sampler);

    BindlessTexturePool _texturePool;
    DescriptorPool _bindlessPool;
    DescriptorSetHandle _globalSet = {};

    std::vector<VkDescriptorSetLayout> _globalSetLayouts;
    std::vector<VkPushConstantRange> _globalPushConstantRanges;
    VkPipelineLayoutHandler _globalLayout;
};
