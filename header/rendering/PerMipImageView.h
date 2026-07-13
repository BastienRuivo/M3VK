#pragma once

#include "allocation/BindingManager.h"
#include "allocation/MultiFrameRessource.h"
#include "rendering/GPUImage.h"
#include "rendering/GraphicsImage.h"
#include <cstdint>
struct PerMipImageView
{
    ImageReference Image;
    std::array<VkImageView, 16> Views;

    void Bind(const BindingManager& manager, uint32_t binding, uint32_t offset) const;
    PerMipImageView() = default;
    PerMipImageView(const ImageReference& image);
    ~PerMipImageView();

    PerMipImageView(PerMipImageView&& other) noexcept;
    PerMipImageView& operator=(PerMipImageView&& other) noexcept;

    PerMipImageView(const PerMipImageView&) = delete;
    PerMipImageView& operator=(const PerMipImageView&) = delete;
};

class  MultiFramePerMipImageView: public MultiFrameRessource<PerMipImageView>
{
public:
    MultiFramePerMipImageView(const BindlessTexture& texture, const BindingManager & manager);
    MultiFramePerMipImageView(const GraphicsImage& image);
    ~MultiFramePerMipImageView() = default;

    void Bind(const BindingManager& manager, uint32_t binding) const;
    inline uint32_t CurrentIndex(uint32_t mipLevel) const { return MultiFrameRessource::CurrentIndex() * ApplicationInfo::Constant::MaxMipCount + mipLevel; }

    MultiFramePerMipImageView(const MultiFramePerMipImageView&) = delete;
    MultiFramePerMipImageView& operator=(const MultiFramePerMipImageView&) = delete;
    MultiFramePerMipImageView(MultiFramePerMipImageView&& other) noexcept;
    MultiFramePerMipImageView& operator=(MultiFramePerMipImageView&& other) noexcept;
};
