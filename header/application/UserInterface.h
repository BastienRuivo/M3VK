#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "rendering/SwapChain.h"

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
