#include "modules/FullscreenDrawDebug.h"
#include "ShaderBindings.h"


FullscreenDrawDebug::FullscreenDrawDebug(ShaderLibrary& shaderLibrary, const BindingManager& manager)
{
    uint32_t fullscreenDrawVertex = shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "FullscreenDraw.vert.spv", ShaderLibrary::Vertex, manager);
    auto& fullscreenDrawVertexInfo = shaderLibrary.Get(fullscreenDrawVertex);
    VertexBinding = {
        .LibraryIndex = fullscreenDrawVertex,
        .Handle = fullscreenDrawVertexInfo.Handle,
        .State = VertexState
        {
            .CullMode = VK_CULL_MODE_NONE
        }
    };

    uint32_t fullscreenDrawFragment = shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "FullscreenDraw.frag.spv", ShaderLibrary::Fragment, manager);
    auto& fullscreenDrawFragmentInfo = shaderLibrary.Get(fullscreenDrawFragment);
    FragmentBinding =
    {
        .LibraryIndex = fullscreenDrawFragment,
        .Handle = fullscreenDrawFragmentInfo.Handle,
        .State = FragmentState
        {
            .depthTest = VK_FALSE,
            .depthWrite = VK_FALSE
        }
    };

    Shader::SpecializationConstant constant{.name = "DRAW_DEPTH", .enabled = true};
    uint32_t fullscreenDrawDepthFragment = shaderLibrary.RegisterShader(std::filesystem::path(SHADER_DIRECTORY) / "FullscreenDraw.frag.spv", ShaderLibrary::Fragment, manager, {&constant, 1});
    auto& fullscreenDrawDepthFragmentInfo = shaderLibrary.Get(fullscreenDrawDepthFragment);
    FragmentDepthBinding = FragmentBinding;
    FragmentDepthBinding.LibraryIndex = fullscreenDrawDepthFragment;
    FragmentDepthBinding.Handle = fullscreenDrawDepthFragmentInfo.Handle;
}

FullscreenDrawDebug::~FullscreenDrawDebug()
{

}

void FullscreenDrawDebug::Draw(const CommandBuffer& cmdBuffer, uint32_t textureIndex, VkPipelineLayout layout, uint32_t mipLevel) const
{
    Draw(cmdBuffer, textureIndex, layout, mipLevel);
}

void FullscreenDrawDebug::Draw(const CommandBuffer& cmdBuffer, DrawOption option, uint32_t textureIndex, VkPipelineLayout layout, uint32_t mipLevel) const
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

        cmdBuffer.PushConstants(layout, sizeof(CommonIndexes), sizeof(TextureConstant), &constant);

        cmdBuffer.Draw(3, 1, 0, 0);
    }
    cmdBuffer.EndMarker();
}

 FullscreenDrawDebug::FullscreenDrawDebug(FullscreenDrawDebug&& other) noexcept
 :  VertexBinding(std::move(other.VertexBinding)),
    FragmentBinding(std::move(other.FragmentBinding)),
    FragmentDepthBinding(std::move(other.FragmentDepthBinding))
 {

 }

FullscreenDrawDebug& FullscreenDrawDebug::operator=(FullscreenDrawDebug&& other) noexcept
{
    if(this != &other)
    {
        VertexBinding = std::move(other.VertexBinding);
        FragmentBinding = std::move(other.FragmentBinding);
        FragmentDepthBinding = std::move(other.FragmentDepthBinding);
    }

    return *this;
}
