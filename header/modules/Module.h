#pragma once

#include "application/UserInterface.h"
#include "rendering/CommandBuffer.h"
class Module
{
public:
    virtual ~Module() {}
    virtual void DoUI(const UserInterface& ui) const {}
    virtual void Resize(const CommandBuffer& cmdBuffer, uint32_t width, uint32_t height) {}
};
