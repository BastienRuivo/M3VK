#version 460

#include "../header/GlobalIncludes.glsl"

layout(push_constant) uniform PushConstants
{
    CommonIndexes indexes;
} push;

// don't forget alignement the day you will have vec2 or nested
layout(set = GLOBAL_SET, binding = BINDING_CAMERA_BUFFER, std430) readonly buffer CameraDataBuffer
{
    CameraData data;
} _Cameras[];


layout (location=0) out vec3 vViewDir;

void main()
{
    uint cameraBufferIndex = push.indexes.Cameras;
    CameraData camera = _Cameras[cameraBufferIndex].data;

    // Generate clip space coordinates:
    // Index 0: (-1, -1) -> Top-Left
    // Index 1: ( 3, -1) -> Top-Right (past boundary)
    // Index 2: (-1,  3) -> Bottom-Left (past boundary)
    vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    vec4 clipPos = vec4(uv * 2.0 - 1.0, 1.0, 1.0);

    vec4 worldPos = camera.InverseViewProjectionMatrixNoTranslation * clipPos;
    vViewDir = worldPos.xyz / worldPos.w;

    gl_Position = clipPos;
}
