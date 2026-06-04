#ifndef LIGHTING_INCLUDE
#define LIGHTING_INCLUDE

#include "../header/GlobalIncludes.glsl"

layout(set = GLOBAL_SET, binding = BINDING_TEXTURES) uniform sampler2D uTextures[];

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

struct LightingInput
{
    vec3 BaseColor;
#ifdef ALPHA_CUTOUT
    float Alpha;
#endif
};

LightingInput CreateLightingInput(uint materialIndex)
{
    MaterialProperties material = _Materials.data[materialIndex];
    vec4 baseColor = texture(uTextures[material.BaseColorTexId], vTexcoords) * material.BaseColor;

    LightingInput lightInput;
    lightInput.BaseColor = baseColor.rgb;
#ifdef ALPHA_CUTOUT
    lightInput.Alpha = baseColor.a;
#endif
    return lightInput;
}

vec3 ComputeLighting(LightingInput lightInput)
{
    return lightInput.BaseColor;
}

#endif
