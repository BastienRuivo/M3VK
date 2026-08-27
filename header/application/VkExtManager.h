#pragma once

#include <vulkan/vulkan.hpp>

class VkExtManager
{
    public:
    static void InitLoader();
    static void InitInstance(vk::Instance instance);
    static void InitDevice(vk::Device device);
};
