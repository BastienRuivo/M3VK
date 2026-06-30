#pragma once


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
    GraphicsImage(DescriptorAllocator& allocator, uint32_t dstBinding, VkSampler sampler, RessourceUsage usage, Args&&... args)
    : GraphicsImage(sampler, usage, std::forward<Args>(args)...)
    {
        for (uint32_t i = 0; i < _images.size(); i++)
        {
            const auto& texture = _images[i];
            allocator.RegisterTexture(ImageHelper::ImageBinding(texture.Internal(), sampler).Descriptor, dstBinding, i);
        }
    }

    template <typename... Args>
    GraphicsImage(VkSampler sampler, RessourceUsage usage, Args&&... args)
    : _sampler(sampler), _usage(usage)
    {
        uint32_t count = usage == RessourceUsage::PerFrame ? ApplicationInfo::Constant::MaxFrameInFlight : 1;
        _images.reserve(count);
        _imGuiSets.reserve(count);

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

    GraphicsImage(GraphicsImage&& other) : _images(std::move(other._images)), _imGuiSets(std::move(other._imGuiSets)), _sampler(other._sampler), _usage(other._usage) {}
    GraphicsImage(const GraphicsImage& other) = delete;

    GraphicsImage& operator=(const GraphicsImage& other) = delete;
    GraphicsImage& operator=(GraphicsImage&& other)
    {
        if (this != &other)
        {
            _images = std::move(other._images);
            _imGuiSets = std::move(other._imGuiSets);
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
    VkDescriptorSet ImGuiSet() const { return _imGuiSets[CurrentIndex()]; }

    ~GraphicsImage() {}

    protected:
    std::vector<GPUImage> _images;
    std::vector<VkDescriptorSet> _imGuiSets;
    VkSampler _sampler;
    RessourceUsage _usage;

    inline const GPUImage& Current() const { return _images[CurrentIndex()]; }
    inline uint32_t CurrentIndex() const { return _usage == RessourceUsage::PerFrame ? ApplicationInfo::CurrentFrame() : 0; }
};
