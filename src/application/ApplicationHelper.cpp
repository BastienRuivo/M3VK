#include "application/ApplicationHelper.h"
#include "glm/ext/quaternion_float.hpp"


#include "application/DebugLayer.h"
#include "application/ApplicationInfo.h"
#include "vulkan/vulkan.hpp"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vulkan/vulkan_raii.hpp>


std::vector<char> ApplicationHelper::ReadFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if(!file.is_open())
    {
        DebugLayer::Log(DebugLayer::LogType::ERROR, "Can't open " + std::string(std::filesystem::current_path()) + "/" + path.string());
        throw std::runtime_error("Can't open file " + path.string());
    }

    size_t fileSize = file.tellg();
    std::vector<char> bytes(fileSize);

    file.seekg(0);
    file.read(bytes.data(), fileSize);
    file.close();

    return bytes;
}

ApplicationHelper::SwapChainSupportDetails ApplicationHelper::QuerySwapChainSupportDetail(vk::PhysicalDevice physicalDevice, const vk::SurfaceKHR& windowSurface)
{
    SwapChainSupportDetails details;
    details.Capabilities = physicalDevice.getSurfaceCapabilitiesKHR(windowSurface);
    details.Formats = physicalDevice.getSurfaceFormatsKHR(windowSurface);
    details.PresentsModes = physicalDevice.getSurfacePresentModesKHR(windowSurface);
    return details;
}

void ApplicationHelper::CopyBufferToBuffer(const vk::Queue queue, const vk::CommandPool& cmdPool, const vk::Buffer& src, vk::DeviceSize srcOffset, const vk::Buffer& dst, vk::DeviceSize dstOffset, vk::DeviceSize size)
{
    vk::Device device = ApplicationInfo::Device();
    vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo{}
        .setCommandPool(cmdPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1);

    vk::CommandBuffer cmdBuffer;
    (void)device.allocateCommandBuffers(&allocInfo, &cmdBuffer);

    vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo{}
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    (void)cmdBuffer.begin(&beginInfo);

    vk::BufferCopy copyRegion = vk::BufferCopy{}
        .setSrcOffset(srcOffset)
        .setDstOffset(dstOffset)
        .setSize(size);
    cmdBuffer.copyBuffer(src, dst, copyRegion);

    cmdBuffer.end();

    vk::CommandBufferSubmitInfo cmdBufferSubmit = vk::CommandBufferSubmitInfo{}
        .setCommandBuffer(cmdBuffer);

    vk::SubmitInfo2 submitInfo = vk::SubmitInfo2{}
        .setCommandBufferInfos(cmdBufferSubmit);

    vk::raii::Fence waitFence(ApplicationInfo::RaiiDevice(), vk::FenceCreateInfo{});

    // wait for the queue idle, we can use a fence to submit multiple shit later
    vk::Result result = queue.submit2(1, &submitInfo, waitFence);
    if(result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to submit command buffer (CopyBufferToBuffer)");
    }

    result = ApplicationInfo::RaiiDevice().waitForFences(*waitFence, vk::True, UINT64_MAX);
    if(result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Wait for single command buffer fence failed (CopyBufferToBuffer)");
    }

    device.freeCommandBuffers(cmdPool, 1, &cmdBuffer);
}

bool ApplicationHelper::IsFormatSupported(vk::Format format, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
    vk::FormatProperties props = ApplicationInfo::PhysicalDevice().getFormatProperties(format);

    if(tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
        return true;
    } else if(tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
        return true;
    }

    return false;
}

vk::ImageAspectFlags ApplicationHelper::GetImageAspectFlags(vk::Format format)
{
    switch(format)
    {
        case vk::Format::eD16Unorm:
        case vk::Format::eD32Sfloat:
        case vk::Format::eD32SfloatS8Uint:
        case vk::Format::eD24UnormS8Uint:
            return vk::ImageAspectFlagBits::eDepth;
        case vk::Format::eS8Uint:
            return vk::ImageAspectFlagBits::eStencil;
        default:
            return vk::ImageAspectFlagBits::eColor;
    }
}

bool ApplicationHelper::HasStencilComponent(vk::Format format)
{
    return format == vk::Format::eS8Uint || format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

uint32_t ApplicationHelper::GetFormatSize(vk::Format format)
{
    switch(format)
    {
        case vk::Format::eD16Unorm:
        case vk::Format::eD32Sfloat:
        case vk::Format::eD32SfloatS8Uint:
        case vk::Format::eD24UnormS8Uint:
            return 4;
        case vk::Format::eS8Uint:
            return 1;
        default: throw std::runtime_error("Unimplemented Format GetFormatSize");
    }
}

glm::quat ApplicationHelper::EulerToQuat(glm::vec3 euler)
{
    return glm::quat(glm::vec3(glm::radians(euler.x), glm::radians(euler.y), glm::radians(euler.z)));
}

glm::mat4 ApplicationHelper::TranslateRotateScale(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale);
    return model;
}
