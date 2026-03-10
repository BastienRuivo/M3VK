#include "header/VkQueueHandler.h"
#include "header/M3VKHelper.h"
#include "header/VkDebugLayer.h"
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

VkQueueHandler::VkQueueHandler(VkDevice device, const M3VKHelper::QueueFamilyIds& queueFamilyIds, VkQueueHandler::QueueTypeEnum queueType)
{
    VkDebugLayer::Log(VkDebugLayer::LogType::CREATE, "VkQueueHandler Creation !");

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
    VkDebugLayer::Log(VkDebugLayer::LogType::DESTROY, "VkQueueHandler Destroyed !");
}
