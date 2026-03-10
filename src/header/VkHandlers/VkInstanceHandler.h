#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

class VkInstanceHandler
{
    public:
    VkInstanceHandler();
    ~VkInstanceHandler();

    VkInstance Get() const;

    private:
    VkInstance _internal = VK_NULL_HANDLE;
    std::vector<const char*> GetRequiredExtensions() const;
};
