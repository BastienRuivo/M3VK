#include "header/VkHandlers/VkCommandPoolHandler.h"
#include "header/DebugLayer.h"
#include "header/ProjectHelper.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

VkCommandPoolHandler::VkCommandPoolHandler(VkDevice device, const ProjectHelper::QueueFamilyIds& families)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkCommandPoolHandler Creation !");
#endif

    _device = device;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = families.Graphics.value();

    if(vkCreateCommandPool(_device, &poolInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool !");
    }
}

VkCommandPool VkCommandPoolHandler::Get() const
{
    return _internal;
}

VkCommandPoolHandler::~VkCommandPoolHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroyCommandPool(_device, _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkCommandPoolHandler Destroyed !");
#endif
}

VkCommandPoolHandler::VkCommandPoolHandler(VkCommandPoolHandler && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkCommandPoolHandler Move Creation !");
#endif

    _internal = other._internal;
    _device = other._device;
    other._internal = VK_NULL_HANDLE;
}

VkCommandPoolHandler& VkCommandPoolHandler::operator=(VkCommandPoolHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        _device = other._device;
        other._internal = VK_NULL_HANDLE;
    }

    return *this;
}
