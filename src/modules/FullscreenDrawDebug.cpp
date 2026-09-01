#include "modules/FullscreenDrawDebug.h"
#include "ShaderBindings.h"


FullscreenDrawDebug::FullscreenDrawDebug(ShaderLibrary& shaderLibrary, const BindingManager& manager)
{
    VertexBinding = shaderLibrary.RegisterVertexBinding(std::filesystem::path(SHADER_DIRECTORY) / "FullscreenDraw.vert.spv", manager,
        VertexState{.CullMode = vk::CullModeFlagBits::eNone});

    FragmentBinding = shaderLibrary.RegisterFragmentBinding(std::filesystem::path(SHADER_DIRECTORY) / "FullscreenDraw.frag.spv", manager,
        FragmentState{.depthTest = VK_FALSE, .depthWrite = VK_FALSE});

    Shader::SpecializationConstant constant{.name = "DRAW_DEPTH", .enabled = true};
    FragmentDepthBinding = shaderLibrary.RegisterFragmentBinding(std::filesystem::path(SHADER_DIRECTORY) / "FullscreenDraw.frag.spv", manager, FragmentBinding.State, {&constant, 1});
}

FullscreenDrawDebug::~FullscreenDrawDebug()
{

}

void FullscreenDrawDebug::Draw(const CommandBuffer& cmdBuffer, uint32_t textureIndex, vk::PipelineLayout layout, uint32_t mipLevel) const
{
    Draw(cmdBuffer, DrawOption_None, textureIndex, layout, mipLevel);
}

void FullscreenDrawDebug::Draw(const CommandBuffer& cmdBuffer, DrawOption option, uint32_t textureIndex, vk::PipelineLayout layout, uint32_t mipLevel) const
{
    cmdBuffer.BeginMarker("Draw Debug");
    {
        VertexBinding.Bind(cmdBuffer);
        if(option == DrawOption_Depth) FragmentDepthBinding.Bind(cmdBuffer);
        else FragmentBinding.Bind(cmdBuffer);

        TextureConstant constant =
        {
            .Index = textureIndex,
            .MipLevel = mipLevel
        };

        cmdBuffer.PushConstants(layout, COMMON_INDEXES_OFFSET, sizeof(TextureConstant), &constant);

        cmdBuffer.Draw(3, 1, 0, 0);
    }
    cmdBuffer.EndMarker();
}
