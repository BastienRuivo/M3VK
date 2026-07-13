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
}

FullscreenDrawDebug::~FullscreenDrawDebug()
{

}

void FullscreenDrawDebug::Draw(const CommandBuffer& cmdBuffer, uint32_t textureIndex, VkPipelineLayout layout, uint32_t mipLevel) const
{
    cmdBuffer.BeginMarker("Draw Debug");
    {
        VertexBinding.Bind(cmdBuffer);
        FragmentBinding.Bind(cmdBuffer);

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
    FragmentBinding(std::move(other.FragmentBinding))
 {

 }

FullscreenDrawDebug& FullscreenDrawDebug::operator=(FullscreenDrawDebug&& other) noexcept
{
    if(this != &other)
    {
        VertexBinding = std::move(other.VertexBinding);
        FragmentBinding = std::move(other.FragmentBinding);
    }

    return *this;
}
