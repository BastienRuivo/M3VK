#ifndef GRAPHICS_BUFFER_APP_CLASS
#define GRAPHICS_BUFFER_APP_CLASS


#include <vulkan/vulkan_core.h>

class StageBuffer
{
    friend class GraphicsBuffer;
    public:
    void Create(const VkPhysicalDevice& physicalDevice, const VkDevice& device, VkDeviceSize size);
    void DisposeBuffer(const VkDevice& device);

    void CopyToBuffer(const VkDevice& device, void* srcData, VkDeviceSize copySize);

    private:
    VkBuffer _buffer;
    VkDeviceMemory _memory;
};


class GraphicsBuffer
{
    public:
    enum BufferType
    {
        INDEX,
        VERTEX
    };
    void Create(const VkPhysicalDevice& physicalDevice, const VkDevice& device, VkDeviceSize size, BufferType type);
    void DisposeBuffer(const VkDevice& device);

    void CopyToBuffer(const VkPhysicalDevice& physicalDevice, const VkDevice& device, const VkQueue& queue, const VkCommandPool& cmdPool, void* srcData, VkDeviceSize size);

    VkBuffer GetInternal() const { return _buffer; }

    private:
    VkBuffer _buffer;
    VkDeviceMemory _memory;
    BufferType _type;
};
#endif
