#ifndef LIGHTING_INCLUDE
#define LIGHTING_INCLUDE

#include "../header/GlobalIncludes.glsl"

layout(push_constant) uniform PushConstants
{
    CommonIndexes indexes;
} push;

#define DEBUG_VERTEX_NORMAL 1
#define DEBUG_NORMAL 2

layout(set = GLOBAL_SET, binding = BINDING_TEXTURES) uniform sampler2D uTextures[];

layout(set = GLOBAL_SET, binding = BINDING_MATERIAL_BUFFER, std430) readonly buffer MaterialPropertiesBuffer
{
    MaterialProperties data[];
} _Materials;

//inputs
layout(location = 0) flat in uint vMaterialIndex;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec4 vTangent;
layout(location=3) in vec2 vTexcoords;
layout(location=4) in vec3 vWsPosition;

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

    // BC5 only stored RG values which let us recontruct B value
    vec2 normalXY = texture(uTextures[material.NormalMapTexId], vTexcoords).rg * 2.0 - 1.0;
    float normalZ = sqrt(max(1.0 - dot(normalXY, normalXY), 0.0));
    vec3 normal = vec3(normalXY, normalZ);


    vec3 N = normalize(vNormal);
    vec3 T = normalize(vTangent.xyz);
    vec3 B = normalize(cross(N, T) * vTangent.w);

    mat3 TBN = mat3(T, B, N);

    vec3 finalNormal = normalize(TBN * normal.xyz);

    LightingInput lightInput;
    lightInput.BaseColor = baseColor.rgb;
    lightInput.Normal = finalNormal;
    lightInput.Alpha = baseColor.a;


    if(ENABLE_CUTOUT && lightInput.Alpha < 0.5) discard;

    return lightInput;
}

vec3 ComputeLighting(LightingInput lightInput)
{
    if(ENABLE_DEBUG)
    {
        if(push.indexes.DebugIndex == DEBUG_VERTEX_NORMAL) return vNormal * 0.5 + 0.5;
        if(push.indexes.DebugIndex == DEBUG_NORMAL) return lightInput.Normal * 0.5 + 0.5;
    }

    vec3 lightDir = normalize(vec3(0, 10000, 10000) - vWsPosition);

    float diffuse = max(max(dot(lightInput.Normal, lightDir), 0.0), 0.1);

    return diffuse * lightInput.BaseColor;
}

#endif
