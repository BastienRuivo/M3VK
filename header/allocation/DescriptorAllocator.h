#pragma once

#include "handler/Handlers.h"
#include "allocation/BindlessTexturePool.h"
#include "allocation/DescriptorPool.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GPUImage.h"
#include <cassert>
#include <cstdint>
#include <vulkan/vulkan_core.h>

class DescriptorAllocator
{
    public:
    struct BindlessTexture
    {
        RessourceUsage usage;
        std::array<uint32_t, ApplicationInfo::Constant::MaxFrameInFlight> index;
        uint32_t StaticIndex() const { assert(usage == RessourceUsage::Static); return index[0]; }

        template<typename... Args>
        static BindlessTexture Register(DescriptorAllocator& allocator, RessourceUsage usage, VkSampler sampler, Args&&... args)
        {
            BindlessTexture handle
            {
                .usage = usage,
                .index = {UINT32_MAX, UINT32_MAX}
            };
            uint32_t count = RessourceUsageCount(usage);
            for (size_t i = 0; i < count; i++)
            {
                GPUImage image = GPUImage(std::forward<Args>(args)...);
                handle.index[i] = allocator.RegisterBindlessTextureInternal(std::move(image), sampler);
            }
            return handle;
        }

        template<typename... Args>
        static BindlessTexture Register(DescriptorAllocator& allocator, RessourceUsage usage, VkSampler sampler, VkCommandPool pool, VkQueue queue, VkImageLayout layout, Args&&... args)
        {
            BindlessTexture handle = BindlessTexture::Register(allocator, usage, sampler, std::forward<Args>(args)...);

            CommandBuffer cmdBuffer(pool, queue);
            cmdBuffer.BeginSingleTime();
            {
                handle.TransistionAllLayoutCommand(allocator, cmdBuffer, layout);
            }
            cmdBuffer.End();
            cmdBuffer.WaitCompletion();

            return handle;
        }

        void Resize(DescriptorAllocator& allocator, uint32_t width, uint32_t height);
        void TransistionAllLayoutCommand(const DescriptorAllocator& allocator, const CommandBuffer& cmdBuffer, VkImageLayout layout);
        inline GPUImage& Texture(DescriptorAllocator& allocator) { return allocator._texturePool.Texture(index[RessourceUsageToCurrentIndex(usage)]); }
        inline GPUImage& PreviousTexture(DescriptorAllocator& allocator) { return allocator._texturePool.Texture(index[RessourceUsageToPreviousIndex(usage)]); }
        void Dispose(DescriptorAllocator& allocator);
    };

    DescriptorAllocator();
    ~DescriptorAllocator();

    static constexpr uint32_t ShaderConstantsSize = 64;

    void RegisterBuffer(const VkDescriptorBufferInfo& info, VkDescriptorType type, uint32_t dstBinding, uint32_t dstArrayElement);
    void RegisterTexture(const VkDescriptorImageInfo& info, uint32_t dstBinding, uint32_t dstArrayElement);

    inline DescriptorSetHandle GlobalDescriptorSet() const { return _globalSet; }
    inline VkPipelineLayout GlobalLayout() const { return _globalLayout.Internal(); }
    inline const std::vector<VkPushConstantRange>& GlobalPushConstantRanges() const { return _globalPushConstantRanges; }
    inline const std::vector<VkDescriptorSetLayout>& GlobalSetLayouts() const { return _globalSetLayouts; }

    private:
    DescriptorSetHandle AllocateBindlessInternal(std::span<uint32_t> counts, VkDescriptorSetLayout layout);
    uint32_t RegisterBindlessTextureInternal(GPUImage&& texture, VkSampler sampler);

    BindlessTexturePool _texturePool;
    DescriptorPool _bindlessPool;
    DescriptorSetHandle _globalSet = {};

    std::vector<VkDescriptorSetLayout> _globalSetLayouts;
    std::vector<VkPushConstantRange> _globalPushConstantRanges;
    VkPipelineLayoutHandler _globalLayout;
};

using BindlessTexture = DescriptorAllocator::BindlessTexture;
