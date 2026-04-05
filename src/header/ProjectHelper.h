#pragma once

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <tiny_obj_loader.h>
#include "DebugLayer.h"
#include "header/Vertex.h"

class ProjectHelper
{
    public:

    static std::vector<char> ReadFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if(!file.is_open())
        {
            DebugLayer::Log(DebugLayer::LogType::ERROR, "Can't open " + std::string(std::filesystem::current_path()) + "/" + filename);
            throw std::runtime_error("Can't open file " + filename);
        }

        size_t fileSize = file.tellg();
        std::vector<char> bytes(fileSize);

        file.seekg(0);
        file.read(bytes.data(), fileSize);
        file.close();

        return bytes;
    }

    struct QueueFamilyIds
    {
        public:
        std::optional<uint32_t> Graphics;
        std::optional<uint32_t> Present;
       // std::optional<uint32_t> Copy;

        static bool AreAllQueueAvailable(const QueueFamilyIds& queueIds)
        {
            return queueIds.Graphics.has_value()
                && queueIds.Present.has_value();
                //&& queueIds.Copy.has_value();
        }
    };

    static QueueFamilyIds QueryQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& windowSurface)
    {
        QueueFamilyIds queueIds;

        uint32_t queueFamiliesCount = 0;

        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(queueFamiliesCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, families.data());

        for(int i = 0; i < families.size(); ++i)
        {
            if(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                queueIds.Graphics = i;
            }
            // else if(families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
            // {
            //     queueIds.Copy = i;
            // }

            VkBool32 isPresentSupported = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, windowSurface, &isPresentSupported);

            if(isPresentSupported)
            {
                queueIds.Present = i;
            }
        }

        return queueIds;
    }

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

    static SwapChainSupportDetails QuerySwapChainSupportDetail(const VkPhysicalDevice& device, const VkSurfaceKHR& windowSurface)
    {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, windowSurface, &details.Capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, windowSurface, &formatCount, nullptr);
        details.Formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, windowSurface, &formatCount, details.Formats.data());

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, windowSurface, &presentModeCount, nullptr);
        if(presentModeCount > 0)
        {
            details.PresentsModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, windowSurface, &presentModeCount, details.PresentsModes.data());
        }
        return details;
    }

    static void CopyBufferToBuffer(const VkDevice& device, const VkQueue queue, const VkCommandPool& cmdPool, const VkBuffer& src, VkDeviceSize srcOffset, const VkBuffer& dst, VkDeviceSize dstOffset, VkDeviceSize size)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = cmdPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer cmdBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(cmdBuffer, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        copyRegion.srcOffset = srcOffset;
        copyRegion.dstOffset = dstOffset;
        vkCmdCopyBuffer(cmdBuffer, src, dst, 1, &copyRegion);

        vkEndCommandBuffer(cmdBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;

        // wait for the queue idle, we can use a fence to submit multiple shit later
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);
    }

    static bool IsFormatSupported(VkPhysicalDevice physicalDevice, VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return true;
        } else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return true;
        }

        return false;
    }

    static VkImageAspectFlags GetImageAspectFlags(VkFormat format)
    {
        switch(format)
        {
            case VK_FORMAT_D16_UNORM:
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
                return VK_IMAGE_ASPECT_DEPTH_BIT;
            case VK_FORMAT_S8_UINT:
                return VK_IMAGE_ASPECT_STENCIL_BIT;
            default:
                return VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }

    static bool HasStencilComponent(VkFormat format)
    {
        return format == VK_FORMAT_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    static void LoadObj(const std::string& path, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err, warn;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn,&err, path.c_str()))
        {
            DebugLayer::Log(DebugLayer::LogType::ERROR, err);
            throw std::runtime_error(err);
        }

        if (warn.empty())
        {
            DebugLayer::Log(DebugLayer::LogType::WARNING, warn);
        }

        indices.reserve(shapes[0].mesh.indices.size());

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for(const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                if(uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }

    }
};
