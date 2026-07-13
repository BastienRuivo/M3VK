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

GraphicsImage::GraphicsImage(GraphicsImage&& other) noexcept
{
    _internals = std::move(other._internals);
    _sampler = std::exchange(other._sampler, VK_NULL_HANDLE);
    _usage = std::exchange(other._usage, RessourceUsage::Static);
}

GraphicsImage& GraphicsImage::operator=(GraphicsImage&& other) noexcept
{
    if (this != &other)
    {
        _internals = std::move(other._internals);
        _sampler = other._sampler;
        _usage = other._usage;
    }
    return *this;
}

void GraphicsImage::TransitionLayout(VkCommandPool pool, VkQueue queue, VkImageLayout newLayout) const
{
    CommandBuffer cmdBuffer(pool, queue);
    cmdBuffer.BeginSingleTime();
    for (auto& image : _internals)
    {
        ImageHelper::TransitionLayoutCommand(cmdBuffer, image.Internal(), VK_IMAGE_LAYOUT_UNDEFINED, newLayout);
    }
    cmdBuffer.End();
    cmdBuffer.WaitCompletion();
}
