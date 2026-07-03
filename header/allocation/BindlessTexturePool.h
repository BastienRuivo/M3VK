#pragma once

#include "rendering/GPUImage.h"
#include <cstdint>
#include <optional>
#include <vector>

class BindlessTexturePool
{
public:
    BindlessTexturePool();
    ~BindlessTexturePool();

    inline GPUImage& Texture(uint32_t textureIndex) { return _textures[textureIndex].value(); }
    inline const GPUImage& Texture(uint32_t textureIndex) const { return _textures[textureIndex].value(); }
    uint32_t Register(GPUImage&& texture);
    void Remove(uint32_t textureIndex);

private:
    inline uint32_t LastFreeTextureIndex() const { return _lastFreeTextureIndex; }
    std::vector<std::optional<GPUImage>> _textures;
    uint32_t _lastFreeTextureIndex = 0;
};
