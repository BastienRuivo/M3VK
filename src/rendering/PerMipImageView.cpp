#include "rendering/PerMipImageView.h"
#include "allocation/BindingManager.h"
#include "application/ApplicationHelper.h"
#include "application/ApplicationInfo.h"
#include <cstdint>
#include <vulkan/vulkan.hpp>


PerMipImageView::PerMipImageView(const ImageReference& image)
{
    Image = image;
    for(uint32_t i = 0; i < image.MipCount; ++i)
    {
        Views[i] = ImageHelper::CreateImageView(Image,
            ApplicationHelper::GetImageAspectFlags(Image.Format),
            (Image.CreateFlags & vk::ImageCreateFlagBits::eCubeCompatible) ? vk::ImageViewType::eCube : vk::ImageViewType::e2D,
            i, 1, 0, image.ArrayLayerCount);
    }
}

PerMipImageView::~PerMipImageView()
{
    for(uint32_t i = 0; i < Image.MipCount; ++i)
    {
        ApplicationInfo::Device().destroyImageView(Views[i]);
    }
}

PerMipImageView::PerMipImageView(PerMipImageView&& other) noexcept
{
    Image = std::exchange(other.Image, {});
    Views = std::exchange(other.Views, {});
}

PerMipImageView& PerMipImageView::operator=(PerMipImageView&& other) noexcept
{
    if(this != &other)
    {
        Image = std::exchange(other.Image, {});
        Views = std::exchange(other.Views, {});
    }
    return *this;
}

void PerMipImageView::Bind(const BindingManager& manager, uint32_t binding, uint32_t offset) const
{
    for(uint32_t i = 0; i < Image.MipCount; ++i)
    {
        vk::DescriptorImageInfo info = vk::DescriptorImageInfo{}
            .setSampler(nullptr)
            .setImageView(Views[i])
            .setImageLayout(vk::ImageLayout::eGeneral);
        manager.RegisterImage(info, binding, i + offset);
    }
}

MultiFramePerMipImageView::MultiFramePerMipImageView(const BindlessTexture& texture, const BindingManager & manager)
{
    _usage = texture.Usage;
    uint32_t count = RessourceUsageCount(texture.Usage);
    _internals.reserve(count);
    for (size_t i = 0; i < count; i++)
    {
        const auto & tex = texture.Texture(manager, i).Internal();
        _internals.emplace_back(tex);
    }
}

void MultiFramePerMipImageView::Bind(const BindingManager& manager, uint32_t binding) const
{
    for (uint32_t i = 0; i < _internals.size(); i++)
    {
        _internals[i].Bind(manager, binding, i * ApplicationInfo::Constant::MaxMipCount);
    }
}
