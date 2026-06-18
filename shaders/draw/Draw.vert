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

// TODO WHEN HERE : CHANGE _INSTANCES TO ARRAY
layout(set = GLOBAL_SET, binding = STATIC_BINDING_INSTANCE_DATA_BUFFER, std430) readonly buffer VisibleInstanceDataBuffer
{
    InstanceData data[];
} _Instances;

layout(set = GLOBAL_SET, binding = BINDING_VISIBLE_INSTANCE_INDIRECTION_BUFFER, std430) readonly buffer VisibleInstanceIndirectionBuffer
{
    uint data[];
} _InstancesIndirection[];


// Note: Data bigger than a vec4 uses multiple location space see https://wikis.khronos.org/opengl/Layout_Qualifier_(GLSL)
// In
layout(location = 0) in vec3 osVertexPosition;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 texCoords;

// Out
layout (location=0) out uint vMaterialIndex;
layout(location=1) out vec3 vNormal;
layout(location=2) out vec3 vTangent;
layout(location=3) out vec2 vTexcoords;
layout(location=4) out vec3 vWsPosition;

void main()
{
    uint cameraBufferIndex = push.indexes.Cameras;
    uint bIndirectionIndex = push.indexes.VisibleInstanceIndirections;

    uint instanceIndex = _InstancesIndirection[bIndirectionIndex].data[gl_InstanceIndex];

    InstanceData instance = _Instances.data[instanceIndex];
    CameraData camera = _Cameras[cameraBufferIndex].data;

    vec4 wsPosition = vec4(osVertexPosition, 1.0);

    vWsPosition = wsPosition.xyz;

    gl_Position = camera.ViewProjectionMatrix * instance.LocalToWorldMatrix * wsPosition;

    vMaterialIndex = instance.MaterialIndex;

    // Correct normal for non uniform scaling
    mat3 normalMatrix = mat3(transpose(inverse(instance.LocalToWorldMatrix)));
    vec3 N = normalize(normalMatrix  * normal);
    vec3 T = normalize(normalMatrix  * tangent);
    T = normalize(T - dot(T, N) * N);

    vNormal = N;
    vTangent = T;

    vTexcoords = texCoords;
}
