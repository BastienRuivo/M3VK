#pragma once

#include "rendering/CommandBuffer.h"
#include "rendering/GPUImage.h"
#include <cstdint>
#include <vulkan/vulkan.hpp>

namespace ImageHelper
{
    struct ImageBinding
    {
        ImageReference Image;
        vk::DescriptorImageInfo Descriptor;
        vk::Sampler Sampler;

        ImageBinding(const ImageReference& image, vk::Sampler sampler, vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal) : Image(image), Sampler(sampler)
        {
            Descriptor = vk::DescriptorImageInfo{}
                .setSampler(sampler)
                .setImageView(image.View)
                .setImageLayout(layout);
        }
        ImageBinding() = default;
    };

    void GenerateMipmapsCommand(const CommandBuffer& cmdBuffer, const ImageReference& image);
    void TransitionLayoutCommand(const CommandBuffer& cmdBuffer, const ImageReference& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
    void TransitionLayoutCommand(const CommandBuffer& cmdBuffer, const ImageReference& image, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
    void StorageImageReadWriteCommand(const CommandBuffer& cmdBuffer, const ImageReference& image, bool isWrite, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, vk::ImageLayout oldLayout = vk::ImageLayout::eGeneral);
    void StorageImageGeneralToLayoutCommand(const CommandBuffer& cmdBuffer, const ImageReference& image, bool isWrite, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, vk::ImageLayout newLayout);

    void CopyToImageCommand(const CommandBuffer& cmdBuffer, const ImageReference& image, uint32_t mipLevel, vk::Buffer srcData);

    inline vk::RenderingAttachmentInfo AttachmentInfo(vk::ImageView imageView, vk::ImageLayout imageLayout, vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp, vk::ClearValue clear = {})
    {
        return vk::RenderingAttachmentInfo{}
            .setImageView(imageView)
            .setImageLayout(imageLayout)
            .setLoadOp(loadOp)
            .setStoreOp(storeOp)
            .setClearValue(clear);
    }

    inline vk::RenderingAttachmentInfo AttachmentInfo(vk::ImageView imageView, vk::ImageLayout imageLayout, vk::ImageView resolveView, vk::ImageLayout resolveLayout, vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp, vk::ResolveModeFlagBits resolveMode, vk::ClearValue clear = {})
    {

        // If there's no MSAA directly render on the resolved image
        if(ApplicationInfo::GetMsaaSample() == vk::SampleCountFlagBits::e1)
        {
            return AttachmentInfo(resolveView, resolveLayout, loadOp, storeOp, clear);
        }

        return AttachmentInfo(imageView, imageLayout, loadOp, storeOp, clear)
            .setResolveMode(resolveMode)
            .setResolveImageView(resolveView)
            .setResolveImageLayout(resolveLayout);
    }

    vk::ImageMemoryBarrier2 TransitionLayoutBarrier(const ImageReference& image, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
    vk::ImageMemoryBarrier2 StorageImageReadWriteBarrier(const ImageReference &image, bool isWrite, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, vk::ImageLayout oldLayout = vk::ImageLayout::eGeneral);
    vk::ImageMemoryBarrier2 StorageImageGeneralToLayoutBarrier(const ImageReference &image, bool isWrite, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, vk::ImageLayout newLayout);
    vk::ImageAspectFlags GetAspect(vk::Format format);
    uint32_t GetMipCount(uint32_t width, uint32_t height);
    uint32_t GetBytePerPixel(vk::Format format);

    vk::ImageView CreateImageView(ImageReference& image, vk::ImageAspectFlags aspectMask, vk::ImageViewType type, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount);
    vk::ImageView CreateImageView(ImageReference& image, vk::ImageAspectFlags aspectMask, vk::ImageViewType type);
}
