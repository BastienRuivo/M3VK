#version 460

#include "../header/GlobalIncludes.glsl"

layout(push_constant) uniform PushConstants
{
    CommonIndexes indexes;
} push;


layout(location=0) out vec2 vTexcoords;

void main()
{
    // Generate clip space coordinates:
    // Index 0: (-1, -1) -> Top-Left
    // Index 1: ( 3, -1) -> Top-Right (past boundary)
    // Index 2: (-1,  3) -> Bottom-Left (past boundary)
    vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    vec4 clipPos = vec4(uv * 2.0 - 1.0, 1.0, 1.0);

    vTexcoords = uv;

    gl_Position = clipPos;
}
