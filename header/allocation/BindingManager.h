#pragma once

#include "handler/Handlers.h"
#include "allocation/BindlessTexturePool.h"
#include "allocation/DescriptorPool.h"
#include "rendering/CommandBuffer.h"
#include "rendering/GPUImage.h"
#include <cassert>
#include <cstdint>
#include <vulkan/vulkan_core.h>

class BindingManager
{
    public:
    struct BindlessTexture
    {
        RessourceUsage Usage;
        std::array<uint32_t, ApplicationInfo::Constant::MaxFrameInFlight> Index;
        VkSampler Sampler;
        uint32_t StaticIndex() const { assert(Usage == RessourceUsage::Static); return Index[0]; }
        uint32_t CurrentBindlessIndex () const { return Index[RessourceUsageToCurrentIndex(Usage)]; }
        uint32_t PreviousBindlessIndex () const { return Index[RessourceUsageToPreviousIndex(Usage)]; }
        uint32_t CurrentIndex() const { return RessourceUsageToCurrentIndex(Usage); }
        uint32_t PreviousIndex() const { return RessourceUsageToPreviousIndex(Usage); }

        template<typename... Args>
        static BindlessTexture Register(BindingManager& allocator, RessourceUsage usage, VkSampler sampler, Args&&... args)
        {
            BindlessTexture handle
            {
                .Usage = usage,
                .Index = {UINT32_MAX, UINT32_MAX},
                .Sampler = sampler
            };
            uint32_t count = RessourceUsageCount(usage);
            for (size_t i = 0; i < count; i++)
            {
                GPUImage image = GPUImage(std::forward<Args>(args)...);
                handle.Index[i] = allocator.RegisterBindlessTextureInternal(std::move(image), handle.Sampler);
            }
            return handle;
        }

        template<typename... Args>
        static BindlessTexture Register(BindingManager& allocator, RessourceUsage usage, VkSampler sampler, VkCommandPool pool, VkQueue queue, VkImageLayout layout, Args&&... args)
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

        bool Resize(BindingManager& allocator, uint32_t width, uint32_t height);
        void TransistionAllLayoutCommand(const BindingManager& allocator, const CommandBuffer& cmdBuffer, VkImageLayout layout);
        inline GPUImage& Texture(BindingManager& allocator) { return allocator._texturePool.Texture(Index[RessourceUsageToCurrentIndex(Usage)]); }
        inline GPUImage& PreviousTexture(BindingManager& allocator) { return allocator._texturePool.Texture(Index[RessourceUsageToPreviousIndex(Usage)]); }
        inline const GPUImage& Texture(const BindingManager& allocator) const { return allocator._texturePool.Texture(Index[RessourceUsageToCurrentIndex(Usage)]); }
        inline const GPUImage& Texture(const BindingManager& allocator, uint32_t texIndex) const { return allocator._texturePool.Texture(Index[texIndex]); }
        inline const GPUImage& PreviousTexture(const BindingManager& allocator) const { return allocator._texturePool.Texture(Index[RessourceUsageToPreviousIndex(Usage)]); }
        void Dispose(BindingManager& allocator);
        void Bind(const BindingManager& allocator, bool hasImage, uint32_t binding, VkSampler sampler) const;
    };

    BindingManager();
    ~BindingManager();

    static constexpr uint32_t ShaderConstantsSize = 64;

    void RegisterBuffer(const VkDescriptorBufferInfo& info, VkDescriptorType type, uint32_t dstBinding, uint32_t dstArrayElement);
    void RegisterTexture(const VkDescriptorImageInfo& info, uint32_t dstBinding, uint32_t dstArrayElement) const;
    void RegisterImage(const VkDescriptorImageInfo& info, uint32_t dstBinding, uint32_t dstArrayElement) const;

    inline DescriptorSetHandle GlobalDescriptorSet() const { return _globalSet; }
    inline VkPipelineLayout GlobalLayout() const { return _globalLayout.Internal(); }
    inline const std::vector<VkPushConstantRange>& GlobalPushConstantRanges() const { return _globalPushConstantRanges; }
    inline const std::vector<VkDescriptorSetLayout>& GlobalSetLayouts() const { return _globalSetLayouts; }

    private:
    DescriptorSetHandle AllocateBindlessInternal(std::span<uint32_t> counts, VkDescriptorSetLayout layout);
    uint32_t RegisterBindlessTextureInternal(GPUImage&& texture, VkSampler sampler);
    void UpdateBindlessTextureInternal(uint32_t index, const ImageReference& img, VkSampler sampler);

    BindlessTexturePool _texturePool;
    DescriptorPool _bindlessPool;
    DescriptorSetHandle _globalSet = {};

    std::vector<VkDescriptorSetLayout> _globalSetLayouts;
    std::vector<VkPushConstantRange> _globalPushConstantRanges;
    VkPipelineLayoutHandler _globalLayout;
};

using BindlessTexture = BindingManager::BindlessTexture;
