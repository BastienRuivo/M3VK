#include "header/VkHandlers/VkFenceHandler.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

VkFenceHandler::VkFenceHandler(VkDevice device)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkFenceHandler Creation !");
#endif
    _device = device;

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // Create the queue in the "Signaled" state to ensure the first frame won't wait eternally for a fence that is not signaled, thus preventing an infinit loop
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Can't create fence");
    }

}

void VkFenceHandler::Wait(uint64_t timeout)
{
    vkWaitForFences(_device, 1, &_internal, VK_TRUE, timeout);
}

void VkFenceHandler::Reset()
{
    vkResetFences(_device, 1, &_internal);
}

VkFenceHandler::~VkFenceHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroyFence(_device, _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkFenceHandler Destroyed !");
#endif
}

VkFenceHandler::VkFenceHandler(VkFenceHandler && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkFenceHandler Move Creation !");
#endif
    _internal = other._internal;
    _device = other._device;
    other._internal = VK_NULL_HANDLE;
}

VkFenceHandler& VkFenceHandler::operator=(VkFenceHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        _device = other._device;
        other._internal = VK_NULL_HANDLE;
    }

    return *this;
}
