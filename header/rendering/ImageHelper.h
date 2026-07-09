#pragma once

#include "rendering/CommandBuffer.h"
#include "rendering/GPUImage.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace ImageHelper
{
    struct ImageBinding
    {
        ImageReference Image;
        VkDescriptorImageInfo Descriptor;
        VkSampler Sampler = VK_NULL_HANDLE;

        ImageBinding(const ImageReference& image, VkSampler sampler, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) : Image(image), Sampler(sampler)
        {
            Descriptor =
            {
                .sampler = sampler,
                .imageView = image.View,
                .imageLayout = layout
            };
        }
        ImageBinding() = default;
    };

    void GenerateMipmapsCommand(const CommandBuffer& cmdBuffer, const ImageReference& image);
    void TransitionLayoutCommand(const CommandBuffer& cmdBuffer, const ImageReference& image, VkImageLayout oldLayout, VkImageLayout newLayout);
    void TransitionLayoutCommand(const CommandBuffer& cmdBuffer, const ImageReference& image, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, VkImageLayout oldLayout, VkImageLayout newLayout);
    void StorageImageReadWriteCommand(const CommandBuffer& cmdBuffer, const ImageReference& image, bool isWrite, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, VkImageLayout oldLayout = VK_IMAGE_LAYOUT_GENERAL);
    void StorageImageGeneralToLayoutCommand(const CommandBuffer& cmdBuffer, const ImageReference& image, bool isWrite, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, VkImageLayout newLayout);

    void CopyToImageCommand(const CommandBuffer& cmdBuffer, const ImageReference& image, uint32_t mipLevel, VkBuffer srcData);

    inline VkRenderingAttachmentInfo AttachmentInfo(VkImageView imageView, VkImageLayout imageLayout, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkClearValue clear = {})
    {
        return VkRenderingAttachmentInfo
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = imageView,
            .imageLayout = imageLayout,
            .loadOp = loadOp,
            .storeOp = storeOp,
            .clearValue = clear
        };
    }

    inline VkRenderingAttachmentInfo AttachmentInfo(VkImageView imageView, VkImageLayout imageLayout, VkImageView resolveView, VkImageLayout resolveLayout, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkClearValue clear = {}, VkResolveModeFlagBits resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT)
    {
        VkRenderingAttachmentInfo info = AttachmentInfo(imageView, imageLayout, loadOp, storeOp, clear);
        info.resolveImageView = resolveView;
        info.resolveImageLayout = resolveLayout;
        info.resolveMode = resolveMode;
        return info;
    }

    VkImageMemoryBarrier2 TransitionLayoutBarrier(const ImageReference& image, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, VkImageLayout oldLayout, VkImageLayout newLayout);
    VkImageMemoryBarrier2 TransitionLayoutBarrier(const VkImageView& image, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, VkImageLayout oldLayout, VkImageLayout newLayout);
    VkImageMemoryBarrier2 StorageImageReadWriteBarrier(const ImageReference &image, bool isWrite, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, VkImageLayout oldLayout = VK_IMAGE_LAYOUT_GENERAL);
    VkImageMemoryBarrier2 StorageImageGeneralToLayoutBarrier(const ImageReference &image, bool isWrite, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount, VkImageLayout newLayout);
    VkImageAspectFlags GetAspect(VkFormat format);
    uint32_t GetMipCount(uint32_t width, uint32_t height);
    uint32_t GetBytePerPixel(VkFormat format);

    VkImageView CreateImageView(ImageReference& image, VkImageAspectFlags aspectMask, VkImageViewType type, uint32_t mipLevel, uint32_t mipCount, uint32_t arrayLayer, uint32_t arrayLayerCount);
    VkImageView CreateImageView(ImageReference& image, VkImageAspectFlags aspectMask, VkImageViewType type);
}
