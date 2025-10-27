#ifndef M3VK_HELPER_CLASS
#define M3VK_HELPER_CLASS

#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>
class M3VKHelper
{
    public:
    struct QueueFamilyId
    {
        public:
        std::optional<uint32_t> Graphics;
        std::optional<uint32_t> Present;

        static bool AreAllQueueAvailable(const QueueFamilyId& queueIds)
        {
            return queueIds.Graphics.has_value()
                && queueIds.Present.has_value();
        }
    };

    static QueueFamilyId QueryQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR& windowSurface)
    {
        QueueFamilyId queueIds;

        uint32_t queueFamiliesCount = 0;

        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(queueFamiliesCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount, families.data());

        for(int i = 0; i < families.size(); ++i)
        {
            if(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                queueIds.Graphics = i;
            }

            VkBool32 isPresentSupported = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, windowSurface, &isPresentSupported);

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
};
#endif
