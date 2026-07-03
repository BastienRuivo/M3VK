#pragma once

#include "rendering/CommandBuffer.h"
#include <vulkan/vulkan_core.h>

class Registry
{
public:
    Registry() = default;
    virtual ~Registry() = default;
    virtual void UploadAndRelease(VkQueue queue, VkCommandPool cmdPool) = 0;
    virtual void Bind(const CommandBuffer& cmdBuffer, VkPipelineLayout pipelineLayout) const {}

    //delete copy constructors
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
};
