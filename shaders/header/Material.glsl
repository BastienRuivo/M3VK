#ifndef MATERIAL_INCLUDE
#define MATERIAL_INCLUDE

struct MaterialProperties
{
    uint BaseColorTexIndex;
    uint NormalTexIndex;
    uint MetallicRoughnessTexIndex;

    vec4 BaseColor;
    float Metallic;
    float Roughness;
};

#endif
