#version 450
#extension GL_EXT_nonuniform_qualifier : enable

#define UINT32_MAX 0xffffffff

struct MaterialProperties
{
    uint BaseColorTexIndex;
    uint NormalTexIndex;
    uint MetallicRoughnessTexIndex;

    vec4 BaseColor;
    float Metallic;
    float Roughness;
};

layout(set = 1, binding = 0) uniform sampler2D uTextures[];
//ssbo
layout(set = 2, binding = 0) readonly buffer MaterialData
{
    MaterialProperties _Material[];
};



//inputs
layout(location = 0) flat in uint vMaterialIndex;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexcoords;

// output
layout(location = 0) out vec4 outColor;

void main()
{
    MaterialProperties material = _Material[vMaterialIndex];

    vec4 baseColor = texture(uTextures[material.BaseColorTexIndex], vTexcoords) * material.BaseColor;

    if(baseColor.a < 0.5) discard;

    outColor = baseColor;
}
