#ifndef M3VK_HELPER_CLASS
#define M3VK_HELPER_CLASS

#include <cstdint>
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
};
#endif
