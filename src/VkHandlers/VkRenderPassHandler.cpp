#include "header/VkHandlers/VkRenderPassHandler.h"
#include "header/SwapChain.h"
#include "header/VkDebugLayer.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

VkRenderPassHandler::VkRenderPassHandler(VkDevice device, VkFormat imageFormat)
{
    _device = device;

    VkAttachmentDescription colorAttachment =  {};
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
    subpass.pipelineBindPoint =VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo rpCreateInfo{};
    rpCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpCreateInfo.attachmentCount = 1;
    rpCreateInfo.pAttachments = &colorAttachment;
    rpCreateInfo.subpassCount = 1;
    rpCreateInfo.pSubpasses = &subpass;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // the passe before or after depending if it's used in src or dst
    dependency.dstSubpass = 0; // our pass

    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;

    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    rpCreateInfo.dependencyCount = 1;
    rpCreateInfo.pDependencies = &dependency;

    if(vkCreateRenderPass(_device, &rpCreateInfo, nullptr, &_internal)  != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create render pass");
    }

    VkDebugLayer::Log(VkDebugLayer::LogType::CREATE, "VkRenderPassHandler Creation !");
}

VkRenderPass VkRenderPassHandler::Get() const
{
    return _internal;
}

VkRenderPassHandler::~VkRenderPassHandler()
{
    vkDestroyRenderPass(_device, _internal, nullptr);
    VkDebugLayer::Log(VkDebugLayer::LogType::DESTROY, "VkRenderPassHandler Destroyed !");
}
