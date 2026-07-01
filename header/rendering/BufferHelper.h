#pragma once

#include "rendering/GraphicsBuffer.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace BufferHelper
{
    struct BufferBinding
    {
        VkDescriptorType DescriptorType;
        VkBuffer Buffer;
        VkDescriptorBufferInfo Descriptor;

        BufferBinding(const GraphicsBuffer& buffer, uint32_t index, uint32_t count = 1)
        {
            Buffer = buffer.Internal();
            DescriptorType = buffer.GetDescriptorType();
            Descriptor = buffer.GetDescriptorBufferInfo(index, count);
        }
    };

    static inline VkDescriptorBufferInfo DescriptorBufferInfo(const GraphicsBuffer& buffer, VkDeviceSize offset)
    {
        return
        {
            .buffer = buffer.Internal(),
            .offset = offset,
            .range = buffer.GetSize()
        };
    }

    static inline VkBufferMemoryBarrier2 BufferBarrier(const GraphicsBuffer& buffer, uint32_t offset,uint32_t size, VkAccessFlagBits2 srcAccessMask, VkPipelineStageFlagBits2 srcStageMask, VkAccessFlagBits2 dstAccessMask, VkPipelineStageFlagBits2 dstStageMask)
    {
        return VkBufferMemoryBarrier2
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = srcStageMask,
            .srcAccessMask = srcAccessMask,
            .dstStageMask = dstStageMask,
            .dstAccessMask = dstAccessMask,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = buffer.Internal(),
            .offset = 0,
            .size = size
        };
    }
}
