#version 450

layout(push_constant, std430) uniform ObjectData
{
    mat4 localToWorldMatrix;
} _Instance;


//inputs
layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec2 vTexcoords;

//ssbo
layout(set = 1, binding = 0) readonly buffer MaterialData
{
    vec4 BaseColor;
} _Material;

layout(set = 1, binding = 1) uniform sampler2D BaseColorTex;
layout(set = 1, binding = 2) uniform sampler2D NormalMapTex;
layout(set = 1, binding = 3) uniform sampler2D MRAOTex;

// output
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(BaseColorTex, vTexcoords) * _Material.BaseColor;
}
