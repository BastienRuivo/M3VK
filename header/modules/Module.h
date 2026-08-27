#pragma once

#include "application/UserInterface.h"
#include "modules/FullscreenDrawDebug.h"
#include "rendering/CommandBuffer.h"
class Module
{
public:
    virtual ~Module() {}
    virtual void DoUI(const UserInterface& ui) {}
    virtual void Resize(const CommandBuffer& cmdBuffer, BindingManager& bindingManager, uint32_t width, uint32_t height) {}
    virtual void RenderUI(const CommandBuffer& cmdBuffer, const FullscreenDrawDebug& debugDrawModule, vk::PipelineLayout layout) const {};
    uint32_t ModuleId = UINT32_MAX;
};
