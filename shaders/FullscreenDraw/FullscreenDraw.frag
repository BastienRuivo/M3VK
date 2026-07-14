#version 460

#include "../header/GlobalIncludes.glsl"

layout(constant_id = 0) const bool DRAW_DEPTH = false;

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
    if(DRAW_DEPTH)
    {
        outColor = vec4(pow(textureLod(globalTextures[push.Constants.TextureIndex], vTexcoords, push.Constants.MipIndex).r, 128.0), 0, 0, 1);
    }
    else
    {
        outColor = textureLod(globalTextures[push.Constants.TextureIndex], vTexcoords, push.Constants.MipIndex);
    }
}
