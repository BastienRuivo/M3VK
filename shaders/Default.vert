#version 460

#include "header/GlobalIncludes.glsl"

layout(push_constant) uniform PushConstants
{
    BufferIndexes bufferIndex;
} push;

// don't forget alignement the day you will have vec2 or nested
layout(set = GLOBAL_SET, binding = BINDING_CAMERA_BUFFER, std430) readonly buffer CameraDataBuffer
{
    CameraData data;
} _Cameras[];

layout(set = GLOBAL_SET, binding = STATIC_BINDING_INSTANCE_DATA_BUFFER, std430) readonly buffer InstanceDataBuffer
{
    InstanceData data[];
} _Instances;


// Note: Vec3 & stuff uses multiple location space see https://wikis.khronos.org/opengl/Layout_Qualifier_(GLSL)
// In
layout(location = 0) in vec3 osVertexPosition;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;

// Out
layout (location=0) out uint vMaterialIndex;
layout(location=1) out vec3 vNormal;
layout(location=2) out vec2 vTexcoords;

void main()
{
    uint cameraBufferIndex = nonuniformEXT(push.bufferIndex.Cameras);

    InstanceData instance = _Instances.data[gl_InstanceIndex];
    CameraData camera = _Cameras[cameraBufferIndex].data;

    gl_Position = camera.ViewProjectionMatrix * instance.LocalToWorldMatrix * vec4(osVertexPosition, 1.0);

    vMaterialIndex = instance.MaterialIndex;
    vNormal = normal;
    vTexcoords = texCoords;
}
