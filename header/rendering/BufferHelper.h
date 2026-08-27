#pragma once

#include "rendering/GraphicsBuffer.h"
#include <cstdint>
#include <vulkan/vulkan.hpp>

namespace BufferHelper
{
    struct BufferBinding
    {
        vk::DescriptorType DescriptorType;
        vk::Buffer Buffer;
        vk::DescriptorBufferInfo Descriptor;

        BufferBinding(const GraphicsBuffer& buffer, uint32_t index, uint32_t count = 1)
        {
            Buffer = buffer.Internal();
            DescriptorType = buffer.GetDescriptorType();
            Descriptor = buffer.GetDescriptorBufferInfo(index, count);
        }
    };

    static inline vk::DescriptorBufferInfo DescriptorBufferInfo(const GraphicsBuffer& buffer, vk::DeviceSize offset)
    {
        return vk::DescriptorBufferInfo{}
            .setBuffer(buffer.Internal())
            .setOffset(offset)
            .setRange(buffer.GetSize());
    }

    static inline vk::BufferMemoryBarrier2 BufferBarrier(const GraphicsBuffer& buffer, uint32_t offset, uint32_t size, vk::AccessFlags2 srcAccessMask, vk::PipelineStageFlags2 srcStageMask, vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 dstStageMask)
    {
        return vk::BufferMemoryBarrier2{}
            .setSrcStageMask(srcStageMask)
            .setSrcAccessMask(srcAccessMask)
            .setDstStageMask(dstStageMask)
            .setDstAccessMask(dstAccessMask)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setBuffer(buffer.Internal())
            .setOffset(0)
            .setSize(size);
    }
}
