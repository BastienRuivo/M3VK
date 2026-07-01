#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>
#include "rendering/QueueFamilyIds.h"
#include "handler/Handlers.h"
#include "handler/VkPhysicalDeviceHandler.h"

class ApplicationInfo
{
    public:

    ApplicationInfo(const ApplicationInfo&) = delete;
    void operator=(const ApplicationInfo&) = delete;

    static ApplicationInfo& Get()
    {
        static ApplicationInfo instance;
        return instance;
    }

    static inline VkInstance Instance() { return ApplicationInfo::Get()._vkInstance; }
    static inline VkDevice Device() { ApplicationInfo::Get(); return ApplicationInfo::Get()._device; }
    static inline VkPhysicalDevice PhysicalDevice() { return ApplicationInfo::Get()._physicalDevice; }

    struct Constant
    {
        static inline constexpr VkSampleCountFlagBits MaxMSAASample = VK_SAMPLE_COUNT_8_BIT;
        static inline constexpr uint32_t MaxFrameInFlight = 2;
        static inline constexpr size_t VertexBufferMaxSize = 16777216; // 2^23
        static inline constexpr size_t IndexBufferMaxSize = 16777216;
        static inline constexpr size_t DrawIndirectBufferMaxSize = 4194304;
        static inline constexpr size_t MaterialBufferMaxSize = 2048;
        static inline constexpr VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT;
        static inline constexpr uint32_t MaxMaterialTextureCount = 1024;
        static inline constexpr uint32_t MaxOtherTextureCount = 1024;
        static inline constexpr uint32_t MaxBufferCount = 1024;
    };

    static inline const QueueFamilyIds& GetQueueFamilyIds()
    {
        return ApplicationInfo::Get()._queueFamilyIds;
    }

    static inline uint32_t GetGraphicsQueueId()
    {
        return ApplicationInfo::Get()._queueFamilyIds.GraphicsCompute.value();
    }

    static inline uint32_t GetPresentQueueId()
    {
        return ApplicationInfo::Get()._queueFamilyIds.Present.value();
    }

    static inline uint32_t GetTransferQueueId()
    {
        return ApplicationInfo::Get()._queueFamilyIds.Transfer.value();
    }

    static inline const VkPhysicalDeviceProperties& GetProperties()
    {
        return ApplicationInfo::Get()._properties;
    }

    static inline VkSampleCountFlagBits GetMsaaSample()
    {
        return ApplicationInfo::Get()._msaaSample;
    }

    static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    static inline uint32_t CurrentFrame() { return ApplicationInfo::Get()._currentFrame; }
    static inline uint32_t PreviousFrame() { return (ApplicationInfo::Get()._currentFrame + Constant::MaxFrameInFlight - 1) % Constant::MaxFrameInFlight; }
    static inline void NextFrame() { ApplicationInfo::Get()._currentFrame = (ApplicationInfo::Get()._currentFrame + 1) % Constant::MaxFrameInFlight; }

    private:
    void SetPhysicalDeviceInformation(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties properties, const QueueFamilyIds& queueFamilyIds);

    ApplicationInfo() {}
    VkSampleCountFlagBits GetMaxUsableSampleCount(VkSampleCountFlagBits maxSample) const;
    QueueFamilyIds _queueFamilyIds;
    VkPhysicalDeviceProperties _properties;
    VkSampleCountFlagBits  _msaaSample = VK_SAMPLE_COUNT_1_BIT;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    VkInstance _vkInstance = VK_NULL_HANDLE;
    uint32_t _currentFrame = 0;

    friend class VkPhysicalDeviceHandler;
    friend class VkDeviceHandler;
    friend class VkInstanceHandler;
};

class VkFunctions
{
    public:
    void LoadVkFunction()
    {
        _isLoaded = false;

        _vkCreateShadersEXT = (PFN_vkCreateShadersEXT)vkGetInstanceProcAddr(ApplicationInfo::Instance(), "vkCreateShadersEXT");
        _vkDestroyShaderEXT = (PFN_vkDestroyShaderEXT)vkGetInstanceProcAddr(ApplicationInfo::Instance(), "vkDestroyShaderEXT");
        _vkCmdBindShadersEXT = (PFN_vkCmdBindShadersEXT)vkGetInstanceProcAddr(ApplicationInfo::Instance(), "vkCmdBindShadersEXT");
        _vkCmdSetColorBlendEnableEXT = (PFN_vkCmdSetColorBlendEnableEXT)vkGetInstanceProcAddr(ApplicationInfo::Instance(), "vkCmdSetColorBlendEnableEXT");
        _vkCmdSetColorBlendEquationEXT = (PFN_vkCmdSetColorBlendEquationEXT)vkGetInstanceProcAddr(ApplicationInfo::Instance(), "vkCmdSetColorBlendEquationEXT");
        _vkCmdSetColorBlendAdvancedEXT = (PFN_vkCmdSetColorBlendAdvancedEXT)vkGetInstanceProcAddr(ApplicationInfo::Instance(), "vkCmdSetColorBlendAdvancedEXT");
        _vkCmdSetColorWriteMaskEXT = (PFN_vkCmdSetColorWriteMaskEXT)vkGetInstanceProcAddr(ApplicationInfo::Instance(), "vkCmdSetColorWriteMaskEXT");
        _vkCmdSetPolygonModeEXT = (PFN_vkCmdSetPolygonModeEXT)vkGetInstanceProcAddr(ApplicationInfo::Instance(), "vkCmdSetPolygonModeEXT");
        _vkCmdSetRasterizationSamplesEXT = (PFN_vkCmdSetRasterizationSamplesEXT)vkGetInstanceProcAddr(ApplicationInfo::Instance(), "vkCmdSetRasterizationSamplesEXT");
        _vkCmdSetSampleMaskEXT = (PFN_vkCmdSetSampleMaskEXT)vkGetInstanceProcAddr(ApplicationInfo::Instance(), "vkCmdSetSampleMaskEXT");
        _vkCmdSetAlphaToCoverageEnableEXT = (PFN_vkCmdSetAlphaToCoverageEnableEXT)vkGetInstanceProcAddr(ApplicationInfo::Instance(), "vkCmdSetAlphaToCoverageEnableEXT");
        _vkCmdSetAlphaToOneEnableEXT = (PFN_vkCmdSetAlphaToOneEnableEXT)vkGetInstanceProcAddr(ApplicationInfo::Instance(), "vkCmdSetAlphaToOneEnableEXT");
        _vkCmdSetDepthClampEnableEXT = (PFN_vkCmdSetDepthClampEnableEXT)vkGetInstanceProcAddr(ApplicationInfo::Instance(), "vkCmdSetDepthClampEnableEXT");
        _vkCmdSetVertexInputEXT = (PFN_vkCmdSetVertexInputEXT)vkGetInstanceProcAddr(ApplicationInfo::Instance(), "vkCmdSetVertexInputEXT");
        _vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(ApplicationInfo::Instance(), "vkCmdBeginDebugUtilsLabelEXT");
        _vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(ApplicationInfo::Instance(), "vkCmdEndDebugUtilsLabelEXT");

        if(_vkCreateShadersEXT == nullptr
            || _vkDestroyShaderEXT == nullptr
            || _vkCmdBindShadersEXT == nullptr
            || _vkCmdSetColorBlendEnableEXT == nullptr
            || _vkCmdSetColorBlendEquationEXT == nullptr
            || _vkCmdSetColorBlendAdvancedEXT == nullptr
            || _vkCmdSetColorWriteMaskEXT == nullptr
            || _vkCmdSetPolygonModeEXT == nullptr
            || _vkCmdSetRasterizationSamplesEXT == nullptr
            || _vkCmdSetSampleMaskEXT == nullptr
            || _vkCmdSetAlphaToCoverageEnableEXT == nullptr
            || _vkCmdSetAlphaToOneEnableEXT == nullptr
            || _vkCmdSetDepthClampEnableEXT == nullptr
            || _vkCmdSetVertexInputEXT == nullptr
        )
        {
            throw std::runtime_error("Function Extension not supported");
        }

        _isLoaded = true;
    }

    static VkFunctions& Instance()
    {
        static VkFunctions instance;
        if(!instance._isLoaded)
        {
            instance.LoadVkFunction();
        }
        return instance;
    }

    inline static VkResult vkCreateShadersEXT(uint32_t createInfoCount, const VkShaderCreateInfoEXT *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkShaderEXT *pShaders)
    {
        auto& Instance = VkFunctions::Instance();
        VkDevice device = ApplicationInfo::Device();
        return Instance._vkCreateShadersEXT(device, createInfoCount, pCreateInfos, nullptr, pShaders);
    }

    inline static void vkDestroyShaderEXT(VkShaderEXT shader, const VkAllocationCallbacks *pAllocator = nullptr)
    {
        auto& Instance = VkFunctions::Instance();
        Instance._vkDestroyShaderEXT(ApplicationInfo::Device(), shader, pAllocator);
    }

    inline static void vkCmdBindShadersEXT(VkCommandBuffer cmdBuffer, uint32_t stageCount, const VkShaderStageFlagBits *pStages, const VkShaderEXT *pShaders)
    {
        auto& Instance = VkFunctions::Instance();
        Instance._vkCmdBindShadersEXT(cmdBuffer, stageCount, pStages, pShaders);
    }

    inline static void vkCmdSetColorBlendEnableEXT(VkCommandBuffer commandBuffer, uint32_t firstAttachment, uint32_t attachmentCount, const VkBool32 *pColorBlendEnables)
    {
        auto& Instance = VkFunctions::Instance();
        Instance._vkCmdSetColorBlendEnableEXT(commandBuffer, firstAttachment, attachmentCount, pColorBlendEnables);
    }

    inline static void vkCmdSetColorBlendEquationEXT(VkCommandBuffer commandBuffer, uint32_t firstAttachment, uint32_t attachmentCount, const VkColorBlendEquationEXT *pColorBlendEquations)
    {
        auto& Instance = VkFunctions::Instance();
        Instance._vkCmdSetColorBlendEquationEXT(commandBuffer, firstAttachment, attachmentCount, pColorBlendEquations);
    }

    inline static void vkCmdSetColorBlendAdvancedEXT(VkCommandBuffer commandBuffer, uint32_t firstAttachment, uint32_t attachmentCount, const VkColorBlendAdvancedEXT *pColorBlendAdvanced)
    {
        auto& Instance = VkFunctions::Instance();
        Instance._vkCmdSetColorBlendAdvancedEXT(commandBuffer, firstAttachment, attachmentCount, pColorBlendAdvanced);
    }

    inline static void vkCmdSetColorWriteMaskEXT(VkCommandBuffer commandBuffer, uint32_t firstAttachment, uint32_t attachmentCount, const VkColorComponentFlags *pColorWriteMasks)
    {
        auto& Instance = VkFunctions::Instance();
        Instance._vkCmdSetColorWriteMaskEXT(commandBuffer, firstAttachment, attachmentCount, pColorWriteMasks);
    }

    inline static void vkCmdSetPolygonModeEXT(VkCommandBuffer commandBuffer, VkPolygonMode polygonMode)
    {
        auto& Instance = VkFunctions::Instance();
        Instance._vkCmdSetPolygonModeEXT(commandBuffer, polygonMode);
    }

    inline static void vkCmdSetRasterizationSamplesEXT(VkCommandBuffer commandBuffer, VkSampleCountFlagBits rasterizationSamples)
    {
        auto& Instance = VkFunctions::Instance();
        Instance._vkCmdSetRasterizationSamplesEXT(commandBuffer, rasterizationSamples);
    }

    inline static void vkCmdSetSampleMaskEXT(VkCommandBuffer commandBuffer, VkSampleCountFlagBits samples, const VkSampleMask *pSampleMask)
    {
        auto& Instance = VkFunctions::Instance();
        Instance._vkCmdSetSampleMaskEXT(commandBuffer, samples, pSampleMask);
    }

    inline static void vkCmdSetAlphaToCoverageEnableEXT(VkCommandBuffer commandBuffer, VkBool32 alphaToCoverageEnable)
    {
        auto& Instance = VkFunctions::Instance();
        Instance._vkCmdSetAlphaToCoverageEnableEXT(commandBuffer, alphaToCoverageEnable);
    }

    inline static void vkCmdSetAlphaToOneEnableEXT(VkCommandBuffer commandBuffer, VkBool32 alphaToOneEnable)
    {
        auto& Instance = VkFunctions::Instance();
        Instance._vkCmdSetAlphaToOneEnableEXT(commandBuffer, alphaToOneEnable);
    }

    inline static void vkCmdSetDepthClampEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthClampEnable)
    {
        auto& Instance = VkFunctions::Instance();
        Instance._vkCmdSetDepthClampEnableEXT(commandBuffer, depthClampEnable);
    }

    inline static void vkCmdSetVertexInputEXT(VkCommandBuffer commandBuffer,
        uint32_t vertexBindingDescriptionCount,
        const VkVertexInputBindingDescription2EXT *pVertexBindingDescriptions,
        uint32_t vertexAttributeDescriptionCount,
        const VkVertexInputAttributeDescription2EXT *pVertexAttributeDescriptions)
    {
        auto& Instance = VkFunctions::Instance();
        Instance._vkCmdSetVertexInputEXT(commandBuffer, vertexBindingDescriptionCount, pVertexBindingDescriptions, vertexAttributeDescriptionCount, pVertexAttributeDescriptions);
    }

    inline static void vkCmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT *pLabelInfo)
    {
        auto& Instance = VkFunctions::Instance();
        Instance._vkCmdBeginDebugUtilsLabelEXT(commandBuffer, pLabelInfo);
    }

    inline static void vkCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer)
    {
        auto& Instance = VkFunctions::Instance();
        Instance._vkCmdEndDebugUtilsLabelEXT(commandBuffer);
    }

    private:
    PFN_vkCreateShadersEXT _vkCreateShadersEXT;
    PFN_vkDestroyShaderEXT _vkDestroyShaderEXT;
    PFN_vkCmdBindShadersEXT _vkCmdBindShadersEXT;
    PFN_vkCmdSetColorBlendEnableEXT _vkCmdSetColorBlendEnableEXT;
    PFN_vkCmdSetColorBlendEquationEXT _vkCmdSetColorBlendEquationEXT;
    PFN_vkCmdSetColorBlendAdvancedEXT _vkCmdSetColorBlendAdvancedEXT;
    PFN_vkCmdSetColorWriteMaskEXT _vkCmdSetColorWriteMaskEXT;
    PFN_vkCmdSetPolygonModeEXT _vkCmdSetPolygonModeEXT;
    PFN_vkCmdSetRasterizationSamplesEXT _vkCmdSetRasterizationSamplesEXT;
    PFN_vkCmdSetSampleMaskEXT _vkCmdSetSampleMaskEXT;
    PFN_vkCmdSetAlphaToCoverageEnableEXT _vkCmdSetAlphaToCoverageEnableEXT;
    PFN_vkCmdSetAlphaToOneEnableEXT _vkCmdSetAlphaToOneEnableEXT;
    PFN_vkCmdSetDepthClampEnableEXT _vkCmdSetDepthClampEnableEXT;
    PFN_vkCmdSetVertexInputEXT _vkCmdSetVertexInputEXT;
    PFN_vkCmdBeginDebugUtilsLabelEXT _vkCmdBeginDebugUtilsLabelEXT;
    PFN_vkCmdEndDebugUtilsLabelEXT _vkCmdEndDebugUtilsLabelEXT;

    bool _isLoaded = false;
};
