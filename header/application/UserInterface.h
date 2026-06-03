#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "application/ApplicationInfo.h"
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

   // widgets
    inline void Begin(const char* windowTitle) const { ImGui::Begin(windowTitle); }
    inline void Text(const char* text) const { ImGui::Text("%s", text); }
    inline void End() const { ImGui::End(); }
    inline void Checkbox(const char* label, bool* value) const  { ImGui::Checkbox(label, value); }

    private:
    VkDescriptorPool _imGuiPool = VK_NULL_HANDLE;
};
