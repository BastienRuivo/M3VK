#include "header/DescriptorPool.h"
#include "header/ApplicationInfo.h"
#include <vector>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

#include <stdexcept>
#include <vulkan/vulkan_core.h>

DescriptorPool::DescriptorPool(std::span<const VkDescriptorSetLayoutBinding> bindings, uint32_t maxSets)
: _maxSets(maxSets)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "DescriptorPool Creation !");
#endif

    VkDescriptorSetLayoutCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    if(vkCreateDescriptorSetLayout(ApplicationInfo::Device(), &createInfo, nullptr, &_layout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout");
    }

    std::vector<VkDescriptorPoolSize> poolSizes(bindings.size());
    for(uint32_t i = 0; i < bindings.size(); ++i)
    {
        poolSizes[i].type = bindings[i].descriptorType;
        poolSizes[i].descriptorCount = bindings[i].descriptorCount * _maxSets;
    }

    VkDescriptorPoolCreateInfo poolInfo
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = maxSets,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    if(vkCreateDescriptorPool(ApplicationInfo::Device(), &poolInfo, nullptr, &_pool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool");
    }

    _allocatedSets = 0;
}

DescriptorPool::~DescriptorPool()
{
#ifdef M3VK_MEMORYLOG
        DebugLayer::Log(DebugLayer::LogType::DESTROY, "DescriptorPool Destroyed !");
#endif
    vkDestroyDescriptorSetLayout(ApplicationInfo::Device(), _layout, nullptr);
    vkDestroyDescriptorPool(ApplicationInfo::Device(), _pool, nullptr);
}

DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept
{
    _layout = other._layout;
    _pool = other._pool;
    other._layout = VK_NULL_HANDLE;
    other._pool = VK_NULL_HANDLE;
}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept
{
    if(this != &other)
    {
        _layout = other._layout;
        _pool = other._pool;
        other._layout = VK_NULL_HANDLE;
        other._pool = VK_NULL_HANDLE;
    }
    return *this;
}

std::vector<VkDescriptorSet> DescriptorPool::Allocate(uint32_t count) const
{
    if(count + _allocatedSets > _maxSets)
    {
        DebugLayer::Log(DebugLayer::LogType::ERROR, "Trying to allocate " + std::to_string(count) + " descriptor sets in a pool with " + std::to_string(_allocatedSets) + " / " + std::to_string(_maxSets) + " occupied slots");
        throw std::runtime_error("Descriptor pool is full !!");
    }

    std::vector<VkDescriptorSetLayout> layouts(count, _layout);
    VkDescriptorSetAllocateInfo allocateInfo
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = _pool,
        .descriptorSetCount = count,
        .pSetLayouts = layouts.data()
    };

    std::vector<VkDescriptorSet> descriptorSets(count);
    if(vkAllocateDescriptorSets(ApplicationInfo::Device(), &allocateInfo, descriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    _allocatedSets += count;
    return descriptorSets;
}

VkDescriptorSet DescriptorPool::Allocate() const
{
    if(_allocatedSets + 1 > _maxSets)
    {
        DebugLayer::Log(DebugLayer::LogType::ERROR, "Trying to allocate 1 descriptor set in a pool with " + std::to_string(_allocatedSets) + " / " + std::to_string(_maxSets) + " occupied slots");
        throw std::runtime_error("Descriptor pool is full !!");
    }

    VkDescriptorSetAllocateInfo allocateInfo
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = _pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &_layout
    };

    VkDescriptorSet descriptorSet;
    if(vkAllocateDescriptorSets(ApplicationInfo::Device(), &allocateInfo, &descriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    _allocatedSets++;
    return descriptorSet;
}

void DescriptorPool::UpdateDescriptorSet(std::span<const VkWriteDescriptorSet> writes, std::span<const VkCopyDescriptorSet> copies) const
{
    vkUpdateDescriptorSets(ApplicationInfo::Device(), static_cast<uint32_t>(writes.size()), writes.data(), static_cast<uint32_t>(copies.size()), copies.data());
}

void DescriptorPool::Free(VkDescriptorSet set) const
{
    vkFreeDescriptorSets(ApplicationInfo::Device(), _pool, 1, &set);
    _allocatedSets--;
}
