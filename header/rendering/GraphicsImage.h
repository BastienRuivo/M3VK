#pragma once


#include "rendering/DescriptorAllocator.h"
#include "rendering/GPUImage.h"
#include "rendering/ImageHelper.h"
#include <cstdint>
class GraphicsImage
{
    public:
    template <typename... Args>
    GraphicsImage(DescriptorAllocator& allocator, uint32_t dstBinding, VkSampler sampler, RessourceUsage usage, Args&&... args)
    : _sampler(sampler)
    {
        _usage = usage;
        uint32_t count = usage == RessourceUsage::PerFrame ? ApplicationInfo::Constant::MaxFrameInFlight : 1;
        _images.reserve(count);

        for (uint32_t i = 0; i < count; i++)
        {
            const auto& texture = _images.emplace_back(std::forward<Args>(args)...);
            allocator.RegisterTexture(ImageHelper::ImageBinding(texture.Internal(), sampler).Descriptor, dstBinding, i);
        }
    }

    inline ImageReference Internal() const { return Current().Internal(); }
    inline VkImageView View() const { return Current().Internal().View; }
    inline uint32_t Width() const { return Current().Internal().Width; }
    inline uint32_t Height() const { return Current().Internal().Height; }
    inline uint32_t MipCount() const { return Current().Internal().MipCount; }

    protected:
    std::vector<GPUImage> _images;
    VkSampler _sampler;
    RessourceUsage _usage;


    const GPUImage& Current() const { return _usage == RessourceUsage::PerFrame ? _images[ApplicationInfo::CurrentFrame()] : _images[0]; }
};
