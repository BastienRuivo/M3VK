#include "header/VkHandlers/VkSamplerHandler.h"
#include "header/VkHandlers/VkPhysicalDeviceHandler.h"
#include <vulkan/vulkan_core.h>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

VkSamplerHandler::VkSamplerHandler(VkDevice device, const VkPhysicalDeviceHandler& physicalDevice)
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkSamplerHandler Creation !");
#endif

    _device = device;

    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    // Mag -> Oversampling, Min -> Undersampling
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;

    // what to do when reading OOB (repeat, clamp, mirror...)
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    // Anisotropy -> Avoiding blur caused by mipmapping by doing clever more sampling
    createInfo.anisotropyEnable = VK_TRUE;
    createInfo.maxAnisotropy = physicalDevice.GetProperties().limits.maxSamplerAnisotropy;

    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;

    // Comparaison operation for shadow mapping apparently
    createInfo.compareEnable = VK_FALSE;
    createInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = VK_LOD_CLAMP_NONE;

    if(vkCreateSampler(_device, &createInfo, nullptr, &_internal) != VK_SUCCESS)
    {
        throw std::runtime_error("Can't create sampler");
    }
}
VkSamplerHandler::~VkSamplerHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroySampler(_device, _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkSamplerHandler Destroyed !");
#endif
}

VkSamplerHandler::VkSamplerHandler(VkSamplerHandler && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkSamplerHandler Move Creation !");
#endif

    _internal = other._internal;
    _device = other._device;
    other._internal = VK_NULL_HANDLE;
    other._device = VK_NULL_HANDLE;
}

VkSamplerHandler& VkSamplerHandler::operator=(VkSamplerHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        _device = other._device;
        other._internal = VK_NULL_HANDLE;
        other._device = VK_NULL_HANDLE;
    }

    return *this;
}
