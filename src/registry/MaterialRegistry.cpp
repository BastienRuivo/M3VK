#include "registry/MaterialRegistry.h"
#include "rendering/GraphicsBuffer.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>


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

    _materialBuffer.CopyToBuffer(queue, cmdPool, _materials.data(), _materials.size() * sizeof(GPUMaterial));
    _materials.clear();
}

void MaterialRegistry::Bind(const CommandBuffer& cmdBuffer) const
{
    //cmdBuffer.BindBuffer(_materialBuffer);
}

MaterialRegistry::MaterialRegistry(size_t materialBufferSize)
: _materialBuffer(materialBufferSize, sizeof(GPUMaterial), GraphicsBuffer::BufferType::STORAGE)
{

}

BufferHelper::BufferBinding MaterialRegistry::Register(GPUMaterial material)
{
    _materials.push_back(material);
    BufferHelper::BufferBinding binding = BufferHelper::BufferBinding(_materialBuffer, _materials.size() - 1);
    return binding;
}
