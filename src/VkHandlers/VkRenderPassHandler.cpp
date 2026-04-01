#include "header/VkHandlers/VkRenderPassHandler.h"
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#ifdef M3VK_MEMORYLOG
#include "header/DebugLayer.h"
#endif

VkRenderPassHandler::VkRenderPassHandler(VkDevice device, VkFormat imageFormat, VkFormat depthFormat)
{
    _device = device;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = imageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // clear at new frame, load to keep and dont care to undefined
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // STORE preserve the data, DONT_CARE otherwise
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // only rendering a triangle, no use of stencil or depth atm
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // Images can have layout suitable for different op
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    // attachment index, only one so 0
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo rpCreateInfo{};
    rpCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    rpCreateInfo.pAttachments = attachments.data();
    rpCreateInfo.subpassCount = 1;
    rpCreateInfo.pSubpasses = &subpass;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // the passe before or after depending if it's used in src or dst
    dependency.dstSubpass = 0; // our pass

    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    rpCreateInfo.dependencyCount = 1;
    rpCreateInfo.pDependencies = &dependency;

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
