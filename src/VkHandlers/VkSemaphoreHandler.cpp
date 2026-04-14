#include "header/VkHandlers/VkSemaphoreHandler.h"
#include "header/ApplicationInfo.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

VkSemaphoreHandler::VkSemaphoreHandler()
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkSemaphoreHandler Creation !");
#endif


    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if(vkCreateSemaphore(ApplicationInfo::Device(), &semaphoreCreateInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Can't create image available semaphore");
    }
}

VkSemaphoreHandler::~VkSemaphoreHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroySemaphore(ApplicationInfo::Device(), _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkSemaphoreHandler Destroyed !");
#endif
}

VkSemaphoreHandler::VkSemaphoreHandler(VkSemaphoreHandler && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkSemaphoreHandler Move Creation !");
#endif

    _internal = other._internal;

    other._internal = VK_NULL_HANDLE;
}

VkSemaphoreHandler& VkSemaphoreHandler::operator=(VkSemaphoreHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;

        other._internal = VK_NULL_HANDLE;
    }

    return *this;
}
