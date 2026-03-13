#include "header/VkHandlers/VkQueueHandler.h"
#include "header/ProjectHelper.h"
#include "header/DebugLayer.h"
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

VkQueueHandler::VkQueueHandler(VkDevice device, const ProjectHelper::QueueFamilyIds& queueFamilyIds, VkQueueHandler::QueueTypeEnum queueType)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkQueueHandler Creation !");
#endif

    uint32_t family;

    switch (queueType)
    {
        case Present: family = queueFamilyIds.Present.value(); break;
        case Graphics: family = queueFamilyIds.Graphics.value(); break;
        default: throw std::runtime_error("Unimplemented graphics queue type");
    }

    vkGetDeviceQueue(device, family, 0, &_internal);
    _queueFamilyIndex = family;
    _type = queueType;
}

VkQueue VkQueueHandler::Get() const
{
    return _internal;
}

uint32_t VkQueueHandler::QueueFamilyId() const
{
    return _queueFamilyIndex;
}

VkQueueHandler::QueueTypeEnum VkQueueHandler::QueueType() const
{
    return _type;
}

VkQueueHandler::~VkQueueHandler()
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkQueueHandler Destroyed !");
#endif
}

VkQueueHandler::VkQueueHandler(VkQueueHandler && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkQueueHandler Move Creation !");
#endif

    // No allocation done on this class, so no need to release the other
    _internal = other._internal;
}

VkQueueHandler& VkQueueHandler::operator=(VkQueueHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
    }

    return *this;
}
