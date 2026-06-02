#ifndef MATERIAL_INCLUDE
#define MATERIAL_INCLUDE

#include "dTypes.h"

struct MaterialProperties
{
    dUint BaseColorTexIndex;
    dUint NormalTexIndex;
    dUint MetallicRoughnessTexIndex;

    dVec4 BaseColor;
    float Metallic;
    float Roughness;
};

#endif
