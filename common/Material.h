#ifndef MATERIAL_INCLUDE
#define MATERIAL_INCLUDE

#include "dTypes.h"

#ifndef GLSL
enum MaterialType
{
    Opaque,
    Cutout,
    CutoutTwoSided,
    Transparent,
    Count
};
#endif

struct MaterialProperties
{
    dAlign(4) dUint BaseColorTexId;
    dAlign(4) dUint NormalMapTexId;
    dAlign(4) dUint MRAOTexId;
    dAlign(4) dUint MaterialType;

    dAlign(16) dVec4 BaseColor;
    dAlign(4) float Metallic;
    dAlign(4) float Roughness;

#ifndef GLSL

    static MaterialProperties Default()
    {
        return
        {
            .BaseColorTexId = UINT32_MAX,
            .NormalMapTexId = UINT32_MAX,
            .MRAOTexId = UINT32_MAX,
            .MaterialType = (uint32_t)MaterialType::Opaque,
            .BaseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .Metallic = 0.0f,
            .Roughness = 1.0f
        };
    }

    bool operator==(const MaterialProperties& other) const
    {
        return BaseColor == other.BaseColor;
    }
#endif
};

#endif
