#pragma once


#include "rendering/CommandBuffer.h"
#include "rendering/DescriptorAllocator.h"
#include "rendering/GPUImage.h"
#include "rendering/ImageHelper.h"
#include "rendering/RessourceUsage.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>
class GraphicsImage
{
    public:

    template <typename... Args>
    GraphicsImage(DescriptorAllocator& allocator, uint32_t dstBinding, VkSampler sampler, RessourceUsage usage, VkCommandPool pool, VkQueue queue, VkImageLayout layout, Args&&... args)
    : GraphicsImage(allocator, dstBinding, sampler, usage, std::forward<Args>(args)...)
    {
        TransitionLayout(pool, queue, layout);
    }

    template <typename... Args>
    GraphicsImage(DescriptorAllocator& allocator, uint32_t dstBinding, VkSampler sampler, RessourceUsage usage, Args&&... args)
    : GraphicsImage(usage, std::forward<Args>(args)...)
    {
        for (uint32_t i = 0; i < _images.size(); i++)
        {
            const auto& texture = _images[i];
            allocator.RegisterTexture(ImageHelper::ImageBinding(texture.Internal(), sampler).Descriptor, dstBinding, i);
        }
    }

    template <typename... Args>
    GraphicsImage(RessourceUsage usage, VkCommandPool pool, VkQueue queue, VkImageLayout layout, Args&&... args)
    : GraphicsImage(usage, std::forward<Args>(args)...)
    {
        TransitionLayout(pool, queue, layout);
    }

    template <typename... Args>
    GraphicsImage(RessourceUsage usage, Args&&... args)
    : _usage(usage)
    {
        uint32_t count = usage == RessourceUsage::PerFrame ? ApplicationInfo::Constant::MaxFrameInFlight : 1;
        _images.reserve(count);

        for (uint32_t i = 0; i < count; i++)
        {
            const auto& texture = _images.emplace_back(std::forward<Args>(args)...);
        }
    }

    void Resize(uint32_t Width, uint32_t Height)
    {
        for(auto & image : _images)
        {
            image.Resize(Width, Height);
        }
    }

    GraphicsImage(GraphicsImage&& other) : _images(std::move(other._images)), _sampler(other._sampler), _usage(other._usage) {}
    GraphicsImage(const GraphicsImage& other) = delete;

    GraphicsImage& operator=(const GraphicsImage& other) = delete;
    GraphicsImage& operator=(GraphicsImage&& other)
    {
        if (this != &other)
        {
            _images = std::move(other._images);
            _sampler = other._sampler;
            _usage = other._usage;
        }
        return *this;
    }

    inline ImageReference Internal() const { return Current().Internal(); }
    inline VkImageView View() const { return Current().Internal().View; }
    inline uint32_t Width() const { return Current().Internal().Width; }
    inline uint32_t Height() const { return Current().Internal().Height; }
    inline uint32_t MipCount() const { return Current().Internal().MipCount; }

    inline ImageReference PreviousInternal() const { return Previous().Internal(); }
    inline VkImageView PreviousView() const { return Previous().Internal().View; }
    inline uint32_t PreviousWidth() const { return Previous().Internal().Width; }
    inline uint32_t PreviousHeight() const { return Previous().Internal().Height; }
    inline uint32_t PreviousMipCount() const { return Previous().Internal().MipCount; }

    ~GraphicsImage() {}

    protected:
    std::vector<GPUImage> _images;
    VkSampler _sampler;
    RessourceUsage _usage;

    inline const GPUImage& Current() const { return _images[CurrentIndex()]; }
    inline const GPUImage& Previous() const { return _images[PreviousIndex()]; }
    inline uint32_t CurrentIndex() const { return _usage == RessourceUsage::PerFrame ? ApplicationInfo::CurrentFrame() : 0; }
    inline uint32_t PreviousIndex() const { return _usage == RessourceUsage::PerFrame ? ApplicationInfo::PreviousFrame() : 0; }

    void TransitionLayout(VkCommandPool pool, VkQueue queue, VkImageLayout newLayout) const
    {
        CommandBuffer cmdBuffer(pool, queue);
        cmdBuffer.BeginSingleTime();
        for (auto& image : _images)
        {
            ImageHelper::TransitionLayoutCommand(cmdBuffer, image.Internal(), VK_IMAGE_LAYOUT_UNDEFINED, newLayout);
        }
        cmdBuffer.End();
        cmdBuffer.WaitCompletion();
    }
};
