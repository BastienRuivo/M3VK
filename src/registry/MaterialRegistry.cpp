#include "registry/MaterialRegistry.h"
#include "application/ApplicationInfo.h"
#include "rendering/DescriptorAllocator.h"
#include "rendering/GPUImage.h"
#include "rendering/GraphicsBuffer.h"
#include "rendering/ImageHelper.h"
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#include "ShaderBindings.h"


void MaterialRegistry::UploadAndRelease(VkQueue queue, VkCommandPool cmdPool)
{
    if(_materials.size() >= ApplicationInfo::Constant::MaterialBufferMaxSize)
    {
        DebugLayer::Log(DebugLayer::LogType::ERROR, "Max buffer size reached");
        throw std::runtime_error("Max buffer size reached");
    }

    if(_materials.size() == 0)
    {
        DebugLayer::Log(DebugLayer::LogType::WARNING, "No materials to upload");
        return;
    }

    _materialBuffer.CopyToBuffer(queue, cmdPool, _materials.data(), _materials.size() * sizeof(MaterialProperties));
    _materials.clear();
}

void MaterialRegistry::Bind(const CommandBuffer& cmdBuffer, VkPipelineLayout layout) const
{
}

uint32_t MaterialRegistry::RegisterMaterial(MaterialProperties material)
{
    material.BaseColorTexId = material.BaseColorTexId == UINT32_MAX ? _materials[0].BaseColorTexId : material.BaseColorTexId;
    material.NormalMapTexId = material.NormalMapTexId == UINT32_MAX ? _materials[0].NormalMapTexId : material.NormalMapTexId;
    material.MRAOTexId = material.MRAOTexId == UINT32_MAX ? _materials[0].MRAOTexId : material.MRAOTexId;

    _materials.push_back(material);
    return _materials.size() - 1;
}

MaterialRegistry::MaterialRegistry(DescriptorAllocator& allocator, uint32_t maxTexturesCount)
    :   _maxTexturesCount(maxTexturesCount),
        _textureIndicesState(std::vector<int32_t>(_maxTexturesCount, -1)),
        _lastFreeTextureIndex(0),
        _materialBuffer(allocator, STATIC_BINDING_MATERIAL_BUFFER, GraphicsBuffer::BufferType::STORAGE, RessourceUsage::Static, ApplicationInfo::Constant::MaterialBufferMaxSize, MaterialProperties::Stride())
{

}

MaterialRegistry::~MaterialRegistry()
{
}

uint32_t MaterialRegistry::RegisterTexture(DescriptorAllocator& allocator, GPUAllocatedImage&& image, VkSampler sampler)
{
    ImageHelper::ImageBinding binding = ImageHelper::ImageBinding(image.Internal(), sampler);
    _textures.emplace_back(std::move(image));

    uint32_t textureIndex = 0;
    bool textureFound = false;
    for (uint32_t i = _lastFreeTextureIndex; i < _textureIndicesState.size() + _lastFreeTextureIndex; i++)
    {
        if (_textureIndicesState[i % _textureIndicesState.size()] == -1)
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

    allocator.RegisterTexture(textureIndex, binding.Descriptor);

    _textureIndicesState[textureIndex] = textureIndex;
    _lastFreeTextureIndex = (textureIndex + 1) % _maxTexturesCount;
    return textureIndex;
}

uint32_t MaterialRegistry::RemoveTexture(uint32_t textureIndex)
{
    _textureIndicesState[textureIndex] = -1;
    return textureIndex;
}
