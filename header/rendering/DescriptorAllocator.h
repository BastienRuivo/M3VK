#pragma once

#include "handler/Handlers.h"
#include "rendering/DescriptorPool.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

class DescriptorAllocator
{
    public:

    DescriptorAllocator();
    ~DescriptorAllocator();

    static constexpr uint32_t ShaderConstantsSize = 64;

    DescriptorSetHandle AllocateBindless(std::span<uint32_t> counts, VkDescriptorSetLayout layout);
    uint32_t RegisterBindlessTexture(uint32_t index, const VkDescriptorImageInfo& imageInfo);
    void RegisterBuffer(const VkDescriptorBufferInfo& info, VkDescriptorType type, uint32_t dstBinding, uint32_t dstArrayElement);
    void RegisterTexture(const VkDescriptorImageInfo& info, uint32_t dstBinding, uint32_t dstArrayElement);

    inline DescriptorSetHandle GlobalDescriptorSet() const { return _globalSet; }
    inline VkPipelineLayout GlobalLayout() const { return _globalLayout.Internal(); }
    inline const std::vector<VkPushConstantRange>& GlobalPushConstantRanges() const { return _globalPushConstantRanges; }
    inline const std::vector<VkDescriptorSetLayout>& GlobalSetLayouts() const { return _globalSetLayouts; }

    private:
    DescriptorPool _bindlessPool;
    DescriptorSetHandle _globalSet = {};

    std::vector<VkDescriptorSetLayout> _globalSetLayouts;
    std::vector<VkPushConstantRange> _globalPushConstantRanges;
    VkPipelineLayoutHandler _globalLayout;
};
