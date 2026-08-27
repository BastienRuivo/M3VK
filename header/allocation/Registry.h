#pragma once

#include "rendering/CommandBuffer.h"
#include <vulkan/vulkan.hpp>

class Registry
{
public:
    Registry() = default;
    virtual ~Registry() = default;
    virtual void UploadAndRelease(vk::Queue queue, vk::CommandPool cmdPool) = 0;
    virtual void Bind(const CommandBuffer& cmdBuffer, vk::PipelineLayout pipelineLayout) const {}

    //delete copy constructors
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
};
