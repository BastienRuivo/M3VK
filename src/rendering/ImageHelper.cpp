#include "rendering/ImageHelper.h"

void ImageHelper::TransitionLayoutCommand(const CommandBuffer& cmdBuffer, const ImageReference& image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    cmdBuffer.TransitionImageLayout(image.Image, image.Format, image.MipCount, oldLayout, newLayout);
}
