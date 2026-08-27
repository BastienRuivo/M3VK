#pragma once

#include "glm/ext/matrix_float4x4.hpp"
#include <cstdint>
#include <filesystem>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ApplicationHelper
{
    struct SwapChainSupportDetails
    {
        public:
        vk::SurfaceCapabilitiesKHR Capabilities;
        std::vector<vk::SurfaceFormatKHR> Formats;
        std::vector<vk::PresentModeKHR> PresentsModes;

        bool CheckSwapChainSupportAdequate()
        {
            return !Formats.empty() && !PresentsModes.empty();
        }
    };

    std::vector<char> ReadFile(const std::filesystem::path& filename);
    SwapChainSupportDetails QuerySwapChainSupportDetail(vk::PhysicalDevice physicalDevice, const vk::SurfaceKHR& windowSurface);
    void CopyBufferToBuffer(const vk::Queue queue, const vk::CommandPool& cmdPool, const vk::Buffer& src, vk::DeviceSize srcOffset, const vk::Buffer& dst, vk::DeviceSize dstOffset, vk::DeviceSize size);
    bool IsFormatSupported(vk::Format format, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    vk::ImageAspectFlags GetImageAspectFlags(vk::Format format);
    bool HasStencilComponent(vk::Format format);
    uint32_t GetFormatSize(vk::Format format);
    glm::quat EulerToQuat(glm::vec3 euler);
    glm::mat4 TranslateRotateScale(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale);
};
