#include "allocation/BindlessTexturePool.h"
#include "application/ApplicationInfo.h"
#include "rendering/GPUImage.h"
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

BindlessTexturePool::BindlessTexturePool()
    :   _lastFreeTextureIndex(0)
{
    _textures.resize(ApplicationInfo::Constant::MaxBindlessTextureCount);
}

BindlessTexturePool::~BindlessTexturePool()
{
}

uint32_t BindlessTexturePool::Register(GPUImage&& image)
{
    uint32_t textureIndex = 0;
    bool textureFound = false;
    for (uint32_t i = _lastFreeTextureIndex; i < _textures.size() + _lastFreeTextureIndex; i++)
    {
        if (!_textures[i % _textures.size()].has_value())
        {
            textureIndex = i;
            textureFound = true;
            break;
        }
    }

    if (!textureFound)
    {
        throw std::runtime_error("BindlessTextureManager is full");
    }

    _textures[textureIndex] = std::move(image);
    _lastFreeTextureIndex = (textureIndex + 1) % ApplicationInfo::Constant::MaxBindlessTextureCount;

    return textureIndex;
}

void BindlessTexturePool::Remove(uint32_t textureIndex)
{
    _textures[textureIndex] = std::nullopt;
}
