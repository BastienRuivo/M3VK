#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "allocation/BindingManager.h"
#include "allocation/MultiFrameRessource.h"
#include "rendering/GraphicsImage.h"
#include "rendering/SwapChain.h"

class UserInterfaceImageSet: public MultiFrameRessource<VkDescriptorSet>
{
    public:
    UserInterfaceImageSet(const GraphicsImage& image, VkImageLayout layout, VkSampler sampler);
    UserInterfaceImageSet(const BindingManager& manager, BindlessTexture texture, VkImageLayout layout, VkSampler sampler);
    ~UserInterfaceImageSet();

    UserInterfaceImageSet(const UserInterfaceImageSet&) = delete;
    UserInterfaceImageSet& operator=(const UserInterfaceImageSet&) = delete;

    UserInterfaceImageSet(UserInterfaceImageSet&& other) noexcept;
    UserInterfaceImageSet& operator=(UserInterfaceImageSet&& other) noexcept;

    inline VkDescriptorSet Current() const { return MultiFrameRessource::Current(); }
};

class UserInterface
{
    public:
    UserInterface(GLFWwindow* pWindow, const SwapChain& swapChain, VkQueue graphicsQueue, VkCommandPool cmdPool);
    ~UserInterface();

    // Workflow function
    void StartFrame() const;
    inline void Render() const { ImGui::Render(); }
    inline void Draw(VkCommandBuffer cmdBuffer) const { ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer); }

    private:
    VkDescriptorPool _imGuiPool = VK_NULL_HANDLE;
};
