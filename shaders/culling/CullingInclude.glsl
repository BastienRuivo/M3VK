#include "../header/GlobalIncludes.glsl"

struct CullingConstants
{
    uint InstanceCount;
    uint DrawCount;
};

layout(push_constant) uniform PushConstants
{
    CommonIndexes indexes;
    CullingConstants _CullingConstants;
} push;

layout(set = GLOBAL_SET, binding = BINDING_CLEAR_DRAW_INDIRECT_BUFFER, std430) readonly buffer DrawIndirectBuffer
{
    DrawIndexedIndirectPadded data[];
} _DrawIndexedIndirect;

layout(set = GLOBAL_SET, binding = BINDING_VISIBLE_DRAW_INDIRECT_BUFFER) writeonly buffer VisibleDrawIndirectBuffer
{
    DrawIndexedIndirectPadded data[];
} _VisibleDrawIndexedIndirect[];
