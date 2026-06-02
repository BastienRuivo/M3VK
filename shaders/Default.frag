#version 460

#include "header/GlobalIncludes.glsl"


layout(set = GLOBAL_SET, binding = BINDING_TEXTURES) uniform sampler2D uTextures[];
//ssbo
layout(set = GLOBAL_SET, binding = STATIC_BINDING_MATERIAL_BUFFER, std430) readonly buffer MaterialPropertiesBuffer
{
    MaterialProperties data[];
} _Materials;

//inputs
layout(location = 0) flat in uint vMaterialIndex;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexcoords;

// output
layout(location = 0) out vec4 outColor;

void main()
{

    MaterialProperties material = _Materials.data[vMaterialIndex];

    vec4 baseColor = texture(uTextures[material.BaseColorTexIndex], vTexcoords) * material.BaseColor;

    if(baseColor.a < 0.5) discard;

    outColor = baseColor;
}
