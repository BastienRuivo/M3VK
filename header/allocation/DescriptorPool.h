#pragma once

#include "application/ApplicationInfo.h"
#include "rendering/GPUImage.h"
#include "allocation/RessourceUsage.h"
#include <cstdint>
#include <span>
#include <vulkan/vulkan_core.h>
#include <vector>

// layout binding
// Layout -> General description
// Pool -> memory pool to allocate something
// Set -> Object allocated on the pool
// So, layout and pool are the memory and need to be cleaned, but as we clean layout, we also clean set automatically

struct DescriptorSetHandle
{
    VkDescriptorSet Set;
    VkDescriptorSetLayout Layout;
    VkDescriptorPool Pool;
};

class DescriptorPool
{
    public:

    class LayoutBuilder
    {
        public:
        LayoutBuilder() = default;

        LayoutBuilder& AddBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags, VkDescriptorBindingFlags bindingFlags, uint32_t count);
        LayoutBuilder& AddBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags, VkDescriptorBindingFlags bindingFlags, RessourceUsage usage);
        inline LayoutBuilder& SetFlags(VkDescriptorSetLayoutCreateFlags flags) { _flags = flags; return *this; }
        VkDescriptorSetLayout Build() const;

        std::vector<VkDescriptorSetLayoutBinding> Bindings;

        private:
        std::vector<VkDescriptorBindingFlags> _bindingFlags;
        VkDescriptorSetLayoutCreateFlags _flags = 0;
    };

    class Builder
    {
        public:
        Builder() = default;

        inline Builder& AddLayout(LayoutBuilder layout) { _layouts.push_back(layout); return *this; }
        inline Builder& SetMaxSets(uint32_t maxSets) { _maxSets = maxSets; return *this; }
        inline Builder& SetFlags(VkDescriptorPoolCreateFlags flags) { _flags = flags; return *this; }
        DescriptorPool Build() const;

        public:
        std::vector<LayoutBuilder> _layouts;
        VkDescriptorPoolCreateFlags _flags = 0;
        uint32_t _maxSets = 0;
    };


    DescriptorPool(std::vector<VkDescriptorSetLayout>&& layouts, VkDescriptorPool pool, uint32_t maxSets);

    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;

    DescriptorPool(DescriptorPool&&) noexcept;
    DescriptorPool& operator=(DescriptorPool&&) noexcept;

    ~DescriptorPool();

    std::vector<DescriptorSetHandle> Allocate(uint32_t layoutIndex, uint32_t count) const;
    DescriptorSetHandle Allocate(uint32_t layoutIndex) const;
    DescriptorSetHandle AllocateBindless(uint32_t layoutIndex, uint32_t count) const;
    void Free(DescriptorSetHandle set) const;

    inline VkDescriptorSetLayout Layout(uint32_t index) const { return _layouts[index]; }
    inline VkDescriptorPool Pool() const { return _pool; }

    static inline VkDescriptorImageInfo DescriptorImageInfo(const ImageReference& image, VkSampler sampler)
    {
        return
        {
            .sampler = sampler,
            .imageView = image.View,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
    }

    private:
    std::vector<VkDescriptorSetLayout> _layouts;
    VkDescriptorPool _pool;
    mutable uint32_t _allocatedSets = 0;
    uint32_t _maxSets;
};


namespace DescriptorHelper
{
    inline void UpdateDescriptorSet(std::span<const VkWriteDescriptorSet> writes, std::span<const VkCopyDescriptorSet> copies)
    {
        vkUpdateDescriptorSets(ApplicationInfo::Device(), static_cast<uint32_t>(writes.size()), writes.data(), static_cast<uint32_t>(copies.size()), copies.data());
    }

    inline void UpdateDescriptorSet(uint32_t writeCount, const VkWriteDescriptorSet* writes, uint32_t copyCount, const VkCopyDescriptorSet* copies)
    {
        vkUpdateDescriptorSets(ApplicationInfo::Device(), writeCount, writes, copyCount, copies);
    }
};
