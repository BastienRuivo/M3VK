#pragma once

#include "header/M3VKHelper.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

class VkQueueHandler
{
    public:
    enum QueueTypeEnum
    {
        Graphics,
        Present,
        Copy
    };

    VkQueueHandler(VkDevice device, const M3VKHelper::QueueFamilyIds& queueFamilyIds, QueueTypeEnum queueType);
    ~VkQueueHandler();

    VkQueue Get() const;
    uint32_t QueueFamilyId() const;
    QueueTypeEnum QueueType() const;

    private:
    VkQueue _internal = VK_NULL_HANDLE;
    uint32_t _queueFamilyIndex;
    enum QueueTypeEnum _type;
};
