#include "header/VkHandlers/Handlers.h"
#include "header/ApplicationInfo.h"
#include <stdexcept>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

VkCommandPoolHandler::VkCommandPoolHandler()
: Handler<VkCommandPool>()
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkCommandPoolHandler Creation !");
#endif

    VkCommandPoolCreateInfo poolInfo
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = ApplicationInfo::Get().GetGraphicsQueueId()
    };

    if(vkCreateCommandPool(ApplicationInfo::Device(), &poolInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool !");
    }
}

VkCommandPoolHandler::~VkCommandPoolHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroyCommandPool(ApplicationInfo::Device(), _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkCommandPoolHandler Destroyed !");
#endif
}
