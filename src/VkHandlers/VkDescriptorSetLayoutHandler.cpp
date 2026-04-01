#include "header/VkHandlers/VkDescriptorSetLayoutHandler.h"
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

VkDescriptorSetLayoutHandler::VkDescriptorSetLayoutHandler(VkDevice device)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkDescriptorSetLayoutHandler Creation !");
#endif

    _device = device;

    VkDescriptorSetLayoutBinding cameraDataLayout{};
    cameraDataLayout.binding = 0;
    cameraDataLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraDataLayout.descriptorCount = 1;
    cameraDataLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    cameraDataLayout.pImmutableSamplers = nullptr; // image sampling

    VkDescriptorSetLayoutBinding imageSamplerLayout{};
    imageSamplerLayout.binding = 1;
    imageSamplerLayout.descriptorCount = 1;
    imageSamplerLayout.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    imageSamplerLayout.pImmutableSamplers = nullptr;
    imageSamplerLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
        cameraDataLayout,
        imageSamplerLayout
    };

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    createInfo.pBindings = bindings.data();

    if(vkCreateDescriptorSetLayout(_device, &createInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
}

VkDescriptorSetLayoutHandler::~VkDescriptorSetLayoutHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroyDescriptorSetLayout(_device, _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkDescriptorSetLayoutHandler Destroyed !");
#endif
}

VkDescriptorSetLayoutHandler::VkDescriptorSetLayoutHandler(VkDescriptorSetLayoutHandler && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkDescriptorSetLayoutHandler Move Creation !");
#endif

    _internal = other._internal;
    _device = other._device;
    other._internal = VK_NULL_HANDLE;
}

VkDescriptorSetLayoutHandler& VkDescriptorSetLayoutHandler::operator=(VkDescriptorSetLayoutHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        _device = other._device;
        other._internal = VK_NULL_HANDLE;
    }

    return *this;
}
