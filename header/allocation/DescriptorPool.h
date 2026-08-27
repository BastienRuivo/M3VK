#pragma once

#include "application/ApplicationInfo.h"
#include "rendering/GPUImage.h"
#include "allocation/RessourceUsage.h"
#include <cstdint>
#include <span>
#include <vulkan/vulkan.hpp>
#include <vector>

// layout binding
// Layout -> General description
// Pool -> memory pool to allocate something
// Set -> Object allocated on the pool
// So, layout and pool are the memory and need to be cleaned, but as we clean layout, we also clean set automatically

struct DescriptorSetHandle
{
    vk::DescriptorSet Set;
    vk::DescriptorSetLayout Layout;
    vk::DescriptorPool Pool;
};

class DescriptorPool
{
    public:

    class LayoutBuilder
    {
        public:
        LayoutBuilder() = default;

        LayoutBuilder& AddBinding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stageFlags, vk::DescriptorBindingFlags bindingFlags, uint32_t count);
        LayoutBuilder& AddBinding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stageFlags, vk::DescriptorBindingFlags bindingFlags, RessourceUsage usage, uint32_t perFrameCount = 1);
        inline LayoutBuilder& SetFlags(vk::DescriptorSetLayoutCreateFlags flags) { _flags = flags; return *this; }
        vk::DescriptorSetLayout Build() const;

        std::vector<vk::DescriptorSetLayoutBinding> Bindings;

        private:
        std::vector<vk::DescriptorBindingFlags> _bindingFlags;
        vk::DescriptorSetLayoutCreateFlags _flags;
    };

    class Builder
    {
        public:
        Builder() = default;

        inline Builder& AddLayout(LayoutBuilder layout) { _layouts.push_back(layout); return *this; }
        inline Builder& SetMaxSets(uint32_t maxSets) { _maxSets = maxSets; return *this; }
        inline Builder& SetFlags(vk::DescriptorPoolCreateFlags flags) { _flags = flags; return *this; }
        DescriptorPool Build() const;

        public:
        std::vector<LayoutBuilder> _layouts;
        vk::DescriptorPoolCreateFlags _flags{};
        uint32_t _maxSets = 0;
    };


    DescriptorPool(std::vector<vk::DescriptorSetLayout>&& layouts, vk::DescriptorPool pool, uint32_t maxSets);

    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;

    DescriptorPool(DescriptorPool&&) noexcept;
    DescriptorPool& operator=(DescriptorPool&&) noexcept;

    ~DescriptorPool();

    std::vector<DescriptorSetHandle> Allocate(uint32_t layoutIndex, uint32_t count) const;
    DescriptorSetHandle Allocate(uint32_t layoutIndex) const;
    DescriptorSetHandle AllocateBindless(uint32_t layoutIndex, uint32_t count) const;
    void Free(DescriptorSetHandle set) const;

    inline vk::DescriptorSetLayout Layout(uint32_t index) const { return _layouts[index]; }
    inline const std::vector<vk::DescriptorSetLayout>& Layouts() const { return _layouts; }
    inline vk::DescriptorPool Pool() const { return _pool; }

    static inline vk::DescriptorImageInfo DescriptorImageInfo(const ImageReference& image, vk::Sampler sampler)
    {
        return vk::DescriptorImageInfo{}
            .setSampler(sampler)
            .setImageView(image.View)
            .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    private:
    std::vector<vk::DescriptorSetLayout> _layouts;
    vk::DescriptorPool _pool;
    mutable uint32_t _allocatedSets = 0;
    uint32_t _maxSets;
};


namespace DescriptorHelper
{
    inline void UpdateDescriptorSet(std::span<const vk::WriteDescriptorSet> writes, std::span<const vk::CopyDescriptorSet> copies)
    {
        ApplicationInfo::Device().updateDescriptorSets(writes, copies);
    }

    inline void UpdateDescriptorSet(uint32_t writeCount, const vk::WriteDescriptorSet* writes, uint32_t copyCount, const vk::CopyDescriptorSet* copies)
    {
        ApplicationInfo::Device().updateDescriptorSets(writeCount, writes, copyCount, copies);
    }
};
