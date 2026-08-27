#include "application/VkExtManager.h"
#include <vulkan/vulkan.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE // exactly one TU in the whole project

void VkExtManager::InitLoader()
{
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
}

void VkExtManager::InitInstance(vk::Instance instance)
{
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
}

void VkExtManager::InitDevice(vk::Device device)
{
    // resolves EXT fn ptrs via vkGetDeviceProcAddr
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
}
