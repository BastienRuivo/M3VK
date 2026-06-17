#version 460

layout(constant_id = 0) const bool ENABLE_CUTOUT = true;
layout(constant_id = 1) const bool ENABLE_DEBUG = false;

#include "Lighting.glsl"

void main()
{
    LightingInput lightingInput = CreateLightingInput(vMaterialIndex);

    outColor = vec4(ComputeLighting(lightingInput), 1.0);
}
