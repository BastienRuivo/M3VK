#include "header/VkHandlers/VkImageHandler.h"
#include <vulkan/vulkan_core.h>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

VkImageHandler::VkImageHandler(VkDevice device, VkImage image, VkFormat format)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkImageHandler Creation !");
#endif

    _device = device;
}
VkImageHandler::~VkImageHandler()
{
    if(_internal == VK_NULL_HANDLE) return;


#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkImageHandler Destroyed !");
#endif
}

VkImageHandler::VkImageHandler(VkImageHandler && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkImageHandler Move Creation !");
#endif

    _internal = other._internal;
    _device = other._device;
    _format = other._format;
    other._internal = VK_NULL_HANDLE;
    other._device = VK_NULL_HANDLE;
    other._format = VK_FORMAT_UNDEFINED;
}

VkImageHandler& VkImageHandler::operator=(VkImageHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        _device = other._device;
        other._internal = VK_NULL_HANDLE;
        other._device = VK_NULL_HANDLE;
        other._format = VK_FORMAT_UNDEFINED;
    }

    return *this;
}
