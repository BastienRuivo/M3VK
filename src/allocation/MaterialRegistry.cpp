#include "allocation/MaterialRegistry.h"
#include "application/ApplicationInfo.h"
#include "asset/MaterialImporter.h"
#include "allocation/BindingManager.h"
#include "rendering/GraphicsBuffer.h"
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

#include "ShaderBindings.h"


void MaterialRegistry::UploadAndRelease(vk::Queue queue, vk::CommandPool cmdPool)
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

void MaterialRegistry::Bind(const CommandBuffer& cmdBuffer, vk::PipelineLayout layout) const
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

MaterialRegistry::MaterialRegistry(BindingManager& allocator, vk::CommandPool uploadPool, vk::Queue uploadQueue, vk::Sampler sampler)
    :   _materialBuffer(allocator, BINDING_MATERIAL_BUFFER, GraphicsBuffer::BufferType::STORAGE, RessourceUsage::Static, ApplicationInfo::Constant::MaterialBufferMaxSize, sizeof(MaterialProperties))
{
    _defaultMaterialIndex = MaterialImporter::LoadDefaultMaterial(allocator, *this, uploadPool, uploadQueue, sampler);
}

MaterialRegistry::~MaterialRegistry()
{
}
