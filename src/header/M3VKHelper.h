#ifndef M3VK_HELPER_CLASS
#define M3VK_HELPER_CLASS

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "VkDebugLayer.h"

class M3VKHelper
{
    public:

    static std::vector<char> ReadFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if(!file.is_open())
        {
            VkDebugLayer::LogError("Can't open " + std::string(std::filesystem::current_path()) + "/" + filename);
            throw std::runtime_error("Can't open file " + filename);
        }

        size_t fileSize = file.tellg();
        std::vector<char> bytes(fileSize);

        file.seekg(0);
        file.read(bytes.data(), fileSize);
        file.close();

        return bytes;
    }

    struct QueueFamilyId
    {
        public:
        std::optional<uint32_t> Graphics;
        std::optional<uint32_t> Present;
       // std::optional<uint32_t> Copy;

        static bool AreAllQueueAvailable(const QueueFamilyId& queueIds)
        {
            return queueIds.Graphics.has_value()
                && queueIds.Present.has_value();
                //&& queueIds.Copy.has_value();
        }
    };

    static QueueFamilyId QueryQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& windowSurface)
    {
        QueueFamilyId queueIds;

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

    static uint32_t FindMemoryType(const VkPhysicalDevice& physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

        for (uint32_t memoryType = 0; memoryType < memoryProperties.memoryTypeCount; ++memoryType)
        {
            // is suitable for buffer & writable by CPU
            if((typeFilter & (1 << memoryType)) && ((memoryProperties.memoryTypes[memoryType].propertyFlags & properties) == properties))
            {
                return memoryType;
            }
        }

        throw std::runtime_error("Can't find suitable memory type for buffer");
    }

    static void CreateBuffer(const VkPhysicalDevice& physicalDevice, const VkDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize memoryOffset = 0) {
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size = size;
        info.usage = usage;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if(vkCreateBuffer(device, &info, nullptr, &buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create buffer !");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memRequirements.size;
        allocateInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        if(vkAllocateMemory(device, &allocateInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate buffer memory");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, memoryOffset);
    }

    static void CopyToBuffer(const VkDevice& device, void* srcData, const VkDeviceMemory& bufferMemory, VkDeviceSize dstOffset, VkDeviceSize copySize)
    {
        void* data;
        vkMapMemory(device, bufferMemory, dstOffset, copySize, 0, &data);
        memcpy(data, srcData, (size_t)copySize);
        vkUnmapMemory(device, bufferMemory);
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
};
#endif
