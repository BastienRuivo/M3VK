#include "header/VkHandlers/VkRenderPassHandler.h"
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

VkRenderPassHandler::VkRenderPassHandler(VkDevice device, VkSampleCountFlagBits sampleCount, VkFormat imageFormat, VkFormat depthFormat)
: _device(device)
{
    VkAttachmentDescription colorAttachment
    {
        .format = imageFormat,
        .samples = sampleCount,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // clear at new frame, load to keep and dont care to undefined
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // STORE preserve the data, DONT_CARE otherwise
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, // only rendering a triangle, no use of stencil or depth atm
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // Images can have layout suitable for different op
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription depthAttachment
    {
        .format = depthFormat,
        .samples = sampleCount,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    // we can't present directly multisampled images, we need to resolve them first
    VkAttachmentDescription colorAttachmentResolve
    {
        .format = imageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    uint32_t attachmentIndex = 0;
    // Multisampled render target
    VkAttachmentReference colorAttachmentRef
    {
        .attachment = attachmentIndex++,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depthAttachmentRef
    {
        .attachment = attachmentIndex++,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    // Resolve MSAA render target
    VkAttachmentReference colorAttachmentResolveRef
    {
        .attachment = attachmentIndex++,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass
    {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pResolveAttachments = &colorAttachmentResolveRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
    };

    std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};

    VkSubpassDependency dependency
    {
        .srcSubpass = VK_SUBPASS_EXTERNAL, // the passe before or after depending if it's used in src or dst
        .dstSubpass = 0, // our pass

        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,

        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo rpCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    if(vkCreateRenderPass(_device, &rpCreateInfo, nullptr, &_internal)  != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create render pass");
    }

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkRenderPassHandler Creation !");
#endif
}

VkRenderPassHandler::~VkRenderPassHandler()
{
    if(_internal == VK_NULL_HANDLE) return;

    vkDestroyRenderPass(_device, _internal, nullptr);

#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::DESTROY, "VkRenderPassHandler Destroyed !");
#endif
}

VkRenderPassHandler::VkRenderPassHandler(VkRenderPassHandler && other) noexcept
{
#ifdef M3VK_MEMORYLOG
    DebugLayer::Log(DebugLayer::LogType::CREATE, "VkRenderPassHandler Move Creation !");
#endif

    _internal = other._internal;
    _device = other._device;
    other._internal = VK_NULL_HANDLE;
}

VkRenderPassHandler& VkRenderPassHandler::operator=(VkRenderPassHandler&& other) noexcept
{
    if(this != &other)
    {
        _internal = other._internal;
        _device = other._device;
        other._internal = VK_NULL_HANDLE;
    }

    return *this;
}
