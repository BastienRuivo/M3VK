#ifndef LIGHTING_INCLUDE
#define LIGHTING_INCLUDE

#include "../header/GlobalIncludes.glsl"

layout(push_constant) uniform PushConstants
{
    CommonIndexes indexes;
} push;

#define DEBUG_NORMAL 1

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
    vec3 Normal;
    float Alpha;
};

LightingInput CreateLightingInput(uint materialIndex)
{
    MaterialProperties material = _Materials.data[materialIndex];
    vec4 baseColor = texture(uTextures[material.BaseColorTexId], vTexcoords) * material.BaseColor;
    #if 0
    vec3 normal = texture(uTextures[material.NormalMapTexId], vTexcoords).rgb;
    #else
    vec3 normal = vNormal;
    #endif

    LightingInput lightInput;
    lightInput.BaseColor = baseColor.rgb;
    lightInput.Normal = normal;
    lightInput.Alpha = baseColor.a;

    return lightInput;
}

vec3 ComputeLighting(LightingInput lightInput)
{
    if(ENABLE_DEBUG && push.indexes.DebugIndex == DEBUG_NORMAL)
    {
        return lightInput.Normal * 0.5 + 0.5;
    }

    return lightInput.BaseColor;
}

#endif
