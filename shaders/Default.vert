#version 450

struct CameraData
{
    mat4 WorldToCameraMatrix;
    mat4 ProjectionMatrix;
    mat4 ViewProjectionMatrix;
};

struct MaterialProperties
{
    uint BaseColorTexIndex;
    uint NormalTexIndex;
    uint MetallicRoughnessTexIndex;
    float Metallic;

    vec4 BaseColor;
    float Roughness;
};

struct ObjectData
{
    mat4 LocalToWorldMatrix;
    uint MaterialIndex;
    uint Pad[3];
};

// don't forget alignement the day you will have vec2 or nested
layout(set = 0, binding = 0) uniform CameraDataBuffer
{
    CameraData _Camera;
};

layout(set = 2, binding = 0, std430) readonly buffer MaterialPropertiesBuffer
{
    MaterialProperties _Material[];
};

layout(set = 2, binding = 1, std430) readonly buffer ObjectDataBuffer
{
    ObjectData _Instance[];
};

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
    ObjectData instance = _Instance[gl_InstanceIndex];
    MaterialProperties material = _Material[instance.MaterialIndex];

    gl_Position = _Camera.ViewProjectionMatrix * instance.LocalToWorldMatrix * vec4(osVertexPosition, 1.0);

    vMaterialIndex = instance.MaterialIndex;
    vNormal = normal;
    vTexcoords = texCoords;
}
