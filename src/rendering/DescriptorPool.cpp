#include "rendering/DescriptorPool.h"
#include "application/ApplicationInfo.h"
#include <cstdint>
#include <utility>
#include <vector>

#ifdef M3VK_MEMORYLOG
#include "application/DebugLayer.h"
#endif

#include <stdexcept>
#include <vulkan/vulkan_core.h>

DescriptorPool::DescriptorPool(std::vector<VkDescriptorSetLayout>&& layouts, VkDescriptorPool pool, uint32_t maxSets)
: _maxSets(maxSets), _layouts(std::move(layouts)), _pool(pool)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "DescriptorPool Creation !");
#endif

    _allocatedSets = 0;
}

DescriptorPool::~DescriptorPool()
{
#ifdef M3VK_MEMORYLOG
        DebugLayer::Log(DebugLayer::LogType::DESTROY, "DescriptorPool Destroyed !");
#endif
    for(auto& layout : _layouts)
    {
        vkDestroyDescriptorSetLayout(ApplicationInfo::Device(), layout, nullptr);
    }
    vkDestroyDescriptorPool(ApplicationInfo::Device(), _pool, nullptr);
}

DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept
{
    _layouts = std::move(other._layouts);
    _pool = std::exchange(other._pool, VK_NULL_HANDLE);
    _allocatedSets = std::exchange(other._allocatedSets, 0);
}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept
{
    if(this != &other)
    {
        _layouts = std::move(other._layouts);
        _pool = std::exchange(other._pool, VK_NULL_HANDLE);
        _allocatedSets = std::exchange(other._allocatedSets, 0);
    }
    return *this;
}

std::vector<DescriptorSetHandle> DescriptorPool::Allocate(uint32_t layoutIndex, uint32_t count) const
{
    if(count + _allocatedSets > _maxSets)
    {
        DebugLayer::Log(DebugLayer::LogType::ERROR, "Trying to allocate " + std::to_string(count) + " descriptor sets in a pool with " + std::to_string(_allocatedSets) + " / " + std::to_string(_maxSets) + " occupied slots");
        throw std::runtime_error("Descriptor pool is full !!");
    }

    std::vector<VkDescriptorSetLayout> layouts(count, _layouts[layoutIndex]);
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

    std::vector<DescriptorSetHandle> handles(count);
    for(size_t i = 0; i < count; i++)
    {
        handles[i] = {descriptorSets[i], _layouts[layoutIndex]};
    }

    _allocatedSets += count;
    return handles;
}

DescriptorSetHandle DescriptorPool::Allocate(uint32_t layoutIndex) const
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
        .pSetLayouts = &_layouts[layoutIndex]
    };

    VkDescriptorSet descriptorSet;
    if(vkAllocateDescriptorSets(ApplicationInfo::Device(), &allocateInfo, &descriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    _allocatedSets++;
    return  {
        .set = descriptorSet,
        .layout = _layouts[layoutIndex],
        .pool = _pool
    };
}

DescriptorSetHandle DescriptorPool::AllocateBindless(uint32_t layoutIndex, uint32_t count) const
{
    if(_allocatedSets + 1 > _maxSets)
    {
        DebugLayer::Log(DebugLayer::LogType::ERROR, "Trying to allocate 1 descriptor set in a pool with " + std::to_string(_allocatedSets) + " / " + std::to_string(_maxSets) + " occupied slots");
        throw std::runtime_error("Descriptor pool is full !!");
    }

    VkDescriptorSetVariableDescriptorCountAllocateInfo countInfo{
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
        .descriptorSetCount = 1,             // must match VkDescriptorSetAllocateInfo::descriptorSetCount
        .pDescriptorCounts  = &count,
    };

    VkDescriptorSetAllocateInfo allocateInfo
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = &countInfo,
        .descriptorPool = _pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &_layouts[layoutIndex]
    };

    VkDescriptorSet descriptorSet;
    if(vkAllocateDescriptorSets(ApplicationInfo::Device(), &allocateInfo, &descriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    _allocatedSets++;
    return  {
        .set = descriptorSet,
        .layout = _layouts[layoutIndex],
        .pool = _pool
    };
}

void DescriptorPool::Free(DescriptorSetHandle set) const
{
    vkFreeDescriptorSets(ApplicationInfo::Device(), _pool, 1, &set.set);
    _allocatedSets--;
}

DescriptorPool::LayoutBuilder& DescriptorPool::LayoutBuilder::AddBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags, VkDescriptorBindingFlags bindingFlags, uint32_t count)
{
    Bindings.push_back(
    {
        .binding = binding,
        .descriptorType = type,
        .descriptorCount = count,
        .stageFlags = stageFlags,
        .pImmutableSamplers = nullptr
    });

    _bindingFlags.push_back(bindingFlags);

    return *this;
}

VkDescriptorSetLayout DescriptorPool::LayoutBuilder::Build() const
{
    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo
    {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount  = static_cast<uint32_t>(_bindingFlags.size()),
        .pBindingFlags = _bindingFlags.data(),
    };

    VkDescriptorSetLayoutCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &flagsInfo,
        .flags = _flags,
        .bindingCount = static_cast<uint32_t>(Bindings.size()),
        .pBindings = Bindings.data()
    };

    VkDescriptorSetLayout layout;
    if(vkCreateDescriptorSetLayout(ApplicationInfo::Device(), &createInfo, nullptr, &layout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout");
    }

    return layout;
}


DescriptorPool DescriptorPool::Builder::Build() const
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::vector<VkDescriptorSetLayout> layouts;
    for(const auto& layout : _layouts)
    {
        bindings.insert(bindings.end(), layout.Bindings.begin(), layout.Bindings.end());
        layouts.push_back(layout.Build());
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
        .pNext = nullptr,
        .flags = _flags,
        .maxSets = _maxSets,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    VkDescriptorPool pool;
    if(vkCreateDescriptorPool(ApplicationInfo::Device(), &poolInfo, nullptr, &pool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool");
    }

    return DescriptorPool(std::move(layouts), pool, _maxSets);
}
