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

    static inline VkBufferMemoryBarrier BufferBarrier(const GraphicsBuffer& buffer, uint32_t offset,uint32_t size, VkAccessFlagBits srcAccessMask, VkAccessFlagBits dstAccessMask)
    {
        return
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = srcAccessMask,
            .dstAccessMask = dstAccessMask,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = buffer.Internal(),
            .offset = 0,
            .size = size
        };
    }
}
