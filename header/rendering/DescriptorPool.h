#pragma once

#include "rendering/GPUImage.h"
#include "rendering/GraphicsBuffer.h"
#include <cstdint>
#include <span>
#include <vulkan/vulkan_core.h>
#include <vector>


// layout binding
// Layout -> General description
// Pool -> memory pool to allocate something
// Set -> Object allocated on the pool
// So, layout and pool are the memory and need to be cleaned, but as we clean layout, we also clean set automatically

class DescriptorPool
{
    public:
    DescriptorPool(std::span<const VkDescriptorSetLayoutBinding> bindings, uint32_t maxSets);

    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;

    DescriptorPool(DescriptorPool&&) noexcept;
    DescriptorPool& operator=(DescriptorPool&&) noexcept;

    ~DescriptorPool();

    std::vector<VkDescriptorSet> Allocate(uint32_t count) const;
    VkDescriptorSet Allocate() const;
    void Free(VkDescriptorSet set) const;

    void UpdateDescriptorSet(std::span<const VkWriteDescriptorSet> writes, std::span<const VkCopyDescriptorSet> copies) const;
    inline VkDescriptorSetLayout Layout() const { return _layout; }
    inline VkDescriptorPool Pool() const { return _pool; }

    static inline VkDescriptorBufferInfo DescriptorBufferInfo(const GraphicsBuffer& buffer, VkDeviceSize offset)
    {
        return
        {
            .buffer = buffer.Internal(),
            .offset = offset,
            .range = buffer.GetSize()
        };
    }

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
    VkDescriptorSetLayout _layout;
    VkDescriptorPool _pool;
    mutable uint32_t _allocatedSets = 0;
    uint32_t _maxSets;
};
