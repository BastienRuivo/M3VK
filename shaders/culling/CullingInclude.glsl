#include "../header/GlobalIncludes.glsl"

struct CullingConstants
{
    uint InstanceCount;
    uint DrawCount;
    uvec2 ScreenSize;
    vec2 ScreenPixelSize;
};

layout(push_constant) uniform PushConstants
{
    GlobalConstants gConstants;
    CullingConstants Constants;
} push;

layout(set = GLOBAL_SET, binding = BINDING_TEXTURES) uniform sampler2D uTextures[];

layout(set = GLOBAL_SET, binding = BINDING_CLEAR_DRAW_INDIRECT_BUFFER, std430) readonly buffer DrawIndirectBuffer
{
    DrawIndexedIndirectPadded data[];
} _DrawIndexedIndirect;

layout(set = GLOBAL_SET, binding = BINDING_VISIBLE_DRAW_INDIRECT_BUFFER) writeonly buffer VisibleDrawIndirectBuffer
{
    DrawIndexedIndirectPadded data[];
} _VisibleDrawIndexedIndirect[];
