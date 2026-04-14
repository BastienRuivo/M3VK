#include "header/VkHandlers/VkQueueHandler.h"
#include "header/ApplicationInfo.h"
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

VkQueueHandler::VkQueueHandler(VkQueueHandler::QueueTypeEnum queueType)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkQueueHandler Creation !");
#endif

    uint32_t family;

    switch (queueType)
    {
        case Present: family = ApplicationInfo::Get().GetPresentQueueId(); break;
        case Graphics: family = ApplicationInfo::Get().GetGraphicsQueueId(); break;
        default: throw std::runtime_error("Unimplemented graphics queue type");
    }

    vkGetDeviceQueue(ApplicationInfo::Device(),family, 0, &_internal);
    _queueFamilyIndex = family;
    _type = queueType;
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
