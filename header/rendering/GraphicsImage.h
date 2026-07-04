#pragma once


#include "rendering/CommandBuffer.h"
#include "allocation/BindingManager.h"
#include "rendering/GPUImage.h"
#include "rendering/ImageHelper.h"
#include "allocation/MultiFrameRessource.h"
#include "rendering/RessourceUsage.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

class GraphicsImage : public MultiFrameRessource<GPUImage>
{
    public:

    template <typename... Args>
    GraphicsImage(BindingManager& allocator, uint32_t dstBinding, VkSampler sampler, RessourceUsage usage, VkCommandPool pool, VkQueue queue, VkImageLayout layout, Args&&... args)
    : GraphicsImage(allocator, dstBinding, sampler, usage, std::forward<Args>(args)...)
    {
        TransitionLayout(pool, queue, layout);
    }

    template <typename... Args>
    GraphicsImage(BindingManager& allocator, uint32_t dstBinding, VkSampler sampler, RessourceUsage usage, Args&&... args)
    : GraphicsImage(usage, std::forward<Args>(args)...)
    {
        for (uint32_t i = 0; i < _internals.size(); i++)
        {
            const auto& texture = _internals[i];
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
    {
        _usage = usage;
        uint32_t count = RessourceUsageCount(usage);
        _internals.reserve(count);

        for (uint32_t i = 0; i < count; i++)
        {
            const auto& texture = _internals.emplace_back(std::forward<Args>(args)...);
        }
    }

    void Resize(uint32_t Width, uint32_t Height);

    GraphicsImage(GraphicsImage&& other) noexcept;
    GraphicsImage(const GraphicsImage& other) = delete;

    GraphicsImage& operator=(const GraphicsImage& other) = delete;
    GraphicsImage& operator=(GraphicsImage&& other) noexcept;

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
    VkSampler _sampler;

    void TransitionLayout(VkCommandPool pool, VkQueue queue, VkImageLayout newLayout) const;
};
