#include "allocation/DescriptorPool.h"
#include "application/ApplicationInfo.h"
#include <cstdint>
#include <utility>
#include <vector>

#include "application/DebugLayer.h"
#include "allocation/RessourceUsage.h"

#include <stdexcept>
#include <vulkan/vulkan.hpp>

DescriptorPool::DescriptorPool(std::vector<vk::DescriptorSetLayout>&& layouts, vk::DescriptorPool pool, uint32_t maxSets)
: _maxSets(maxSets), _layouts(std::move(layouts)), _pool(pool)
{
    _allocatedSets = 0;
}

DescriptorPool::~DescriptorPool()
{
    vk::Device device = ApplicationInfo::Device();
    for(auto& layout : _layouts)
    {
        device.destroyDescriptorSetLayout(layout);
    }
    device.destroyDescriptorPool(_pool);
}

DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept
{
    _layouts = std::move(other._layouts);
    _pool = std::exchange(other._pool, nullptr);
    _allocatedSets = std::exchange(other._allocatedSets, 0);
}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept
{
    if(this != &other)
    {
        _layouts = std::move(other._layouts);
        _pool = std::exchange(other._pool, nullptr);
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

    std::vector<vk::DescriptorSetLayout> layouts(count, _layouts[layoutIndex]);
    vk::DescriptorSetAllocateInfo allocateInfo = vk::DescriptorSetAllocateInfo{}
        .setDescriptorPool(_pool)
        .setSetLayouts(layouts);

    std::vector<vk::DescriptorSet> descriptorSets(count);
    if(ApplicationInfo::Device().allocateDescriptorSets(&allocateInfo, descriptorSets.data()) != vk::Result::eSuccess)
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

    vk::DescriptorSetAllocateInfo allocateInfo = vk::DescriptorSetAllocateInfo{}
        .setDescriptorPool(_pool)
        .setSetLayouts(_layouts[layoutIndex]);

    vk::DescriptorSet descriptorSet;
    if(ApplicationInfo::Device().allocateDescriptorSets(&allocateInfo, &descriptorSet) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    _allocatedSets++;
    return  {
        .Set = descriptorSet,
        .Layout = _layouts[layoutIndex],
        .Pool = _pool
    };
}

void DescriptorPool::Free(DescriptorSetHandle set) const
{
    if(ApplicationInfo::Device().freeDescriptorSets(_pool, 1, &set.Set) != vk::Result::eSuccess)
    {
        DebugLayer::Log(DebugLayer::WARNING, "Failed to free descriptor set");
    }
    _allocatedSets--;
}

DescriptorPool::LayoutBuilder& DescriptorPool::LayoutBuilder::AddBinding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stageFlags, vk::DescriptorBindingFlags bindingFlags, uint32_t count)
{
    Bindings.push_back(vk::DescriptorSetLayoutBinding{}
        .setBinding(binding)
        .setDescriptorType(type)
        .setDescriptorCount(count)
        .setStageFlags(stageFlags));

    _bindingFlags.push_back(bindingFlags);

    return *this;
}

DescriptorPool::LayoutBuilder& DescriptorPool::LayoutBuilder::AddBinding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stageFlags, vk::DescriptorBindingFlags bindingFlags, RessourceUsage usage, uint32_t perFrameCount)
{
    uint32_t count = RessourceUsageCount(usage);

    return AddBinding(binding, type, stageFlags, bindingFlags, count * perFrameCount);
}

vk::DescriptorSetLayout DescriptorPool::LayoutBuilder::Build() const
{
    vk::DescriptorSetLayoutBindingFlagsCreateInfo flagsInfo = vk::DescriptorSetLayoutBindingFlagsCreateInfo{}
        .setBindingFlags(_bindingFlags);

    vk::DescriptorSetLayoutCreateInfo createInfo = vk::DescriptorSetLayoutCreateInfo{}
        .setPNext(&flagsInfo)
        .setFlags(_flags)
        .setBindings(Bindings);

    vk::DescriptorSetLayout layout;
    if(ApplicationInfo::Device().createDescriptorSetLayout(&createInfo, nullptr, &layout) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create descriptor set layout");
    }

    return layout;
}


DescriptorPool DescriptorPool::Builder::Build() const
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    std::vector<vk::DescriptorSetLayout> layouts;
    for(const auto& layout : _layouts)
    {
        bindings.insert(bindings.end(), layout.Bindings.begin(), layout.Bindings.end());
        layouts.push_back(layout.Build());
    }

    std::vector<vk::DescriptorPoolSize> poolSizes(bindings.size());
    for(uint32_t i = 0; i < bindings.size(); ++i)
    {
        poolSizes[i].type = bindings[i].descriptorType;
        poolSizes[i].descriptorCount = bindings[i].descriptorCount * _maxSets;
    }

    vk::DescriptorPoolCreateInfo poolInfo = vk::DescriptorPoolCreateInfo{}
        .setFlags(_flags)
        .setMaxSets(_maxSets)
        .setPoolSizes(poolSizes);

    vk::DescriptorPool pool;
    if(ApplicationInfo::Device().createDescriptorPool(&poolInfo, nullptr, &pool) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create descriptor pool");
    }

    return DescriptorPool(std::move(layouts), pool, _maxSets);
}
