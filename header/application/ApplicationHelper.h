#pragma once

#include "glm/ext/matrix_float4x4.hpp"
#include <cstdint>
#include <filesystem>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <glm/gtc/quaternion.hpp>

namespace ApplicationHelper
{
    struct SwapChainSupportDetails
    {
        public:
        VkSurfaceCapabilitiesKHR Capabilities;
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentsModes;

        bool CheckSwapChainSupportAdequate()
        {
            return !Formats.empty() && !PresentsModes.empty();
        }
    };

    std::vector<char> ReadFile(const std::filesystem::path& filename);
    SwapChainSupportDetails QuerySwapChainSupportDetail(VkPhysicalDevice physicalDevice, const VkSurfaceKHR& windowSurface);
    void CopyBufferToBuffer(const VkQueue queue, const VkCommandPool& cmdPool, const VkBuffer& src, VkDeviceSize srcOffset, const VkBuffer& dst, VkDeviceSize dstOffset, VkDeviceSize size);
    bool IsFormatSupported(VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkImageAspectFlags GetImageAspectFlags(VkFormat format);
    bool HasStencilComponent(VkFormat format);
    uint32_t GetFormatSize(VkFormat format);
    glm::quat EulerToQuat(glm::vec3 euler);
    glm::mat4 TranslateRotateScale(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale);
};
