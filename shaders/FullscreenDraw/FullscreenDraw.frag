#version 460

#include "../header/GlobalIncludes.glsl"

struct TextureConstant
{
    uint TextureIndex;
    uint MipIndex;
};

layout(push_constant) uniform PushConstants
{
    CommonIndexes indexes;
    TextureConstant Constants;
} push;

layout(location = 0) in vec2 vTexcoords;
layout(location = 0) out vec4 outColor;

layout(set = GLOBAL_SET, binding = BINDING_TEXTURES) uniform sampler2D globalTextures[];

void main()
{
    outColor = textureLod(globalTextures[push.Constants.TextureIndex], vTexcoords, push.Constants.MipIndex);
}
