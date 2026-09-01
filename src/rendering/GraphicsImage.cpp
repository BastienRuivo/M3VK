#include "rendering/GraphicsImage.h"

bool GraphicsImage::Resize(uint32_t Width, uint32_t Height)
{
    bool hasResize = false;
    for(auto & image : _internals)
    {
        hasResize |= image.Resize(Width, Height);
    }
    return hasResize;
}

void GraphicsImage::TransitionLayout(vk::CommandPool pool, vk::Queue queue, vk::ImageLayout newLayout) const
{
    CommandBuffer cmdBuffer(pool, queue);
    cmdBuffer.BeginSingleTime();
    for (auto& image : _internals)
    {
        ImageHelper::TransitionLayoutCommand(cmdBuffer, image.Internal(), vk::ImageLayout::eUndefined, newLayout);
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
}
