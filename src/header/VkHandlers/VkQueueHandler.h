#pragma once

#include "header/ProjectHelper.h"
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

    VkQueueHandler(VkDevice device, const ProjectHelper::QueueFamilyIds& queueFamilyIds, QueueTypeEnum queueType);
    ~VkQueueHandler();

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    VkQueueHandler(VkQueueHandler&& other) noexcept;
    VkQueueHandler& operator=(VkQueueHandler&& other) noexcept;

    VkQueueHandler(const VkQueueHandler&) = delete;
    VkQueueHandler& operator=(const VkQueueHandler&) = delete;

    inline VkQueue Get() const
    {
        return _internal;
    }
    uint32_t QueueFamilyId() const;
    QueueTypeEnum QueueType() const;

    private:
    VkQueue _internal = VK_NULL_HANDLE;
    uint32_t _queueFamilyIndex;
    enum QueueTypeEnum _type;
};
