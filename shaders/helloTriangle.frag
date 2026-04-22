#version 450

layout(push_constant, std430) uniform ObjectData
{
    mat4 localToWorldMatrix;
} _Instance;


//inputs
layout(location = 0) in vec2 fragTexCoords;

//ssbo
layout(set = 1, binding = 0) readonly buffer MaterialData
{
    vec4 Albedo;
} _Material;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

// output
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(texSampler, fragTexCoords) * _Material.Albedo;
}
