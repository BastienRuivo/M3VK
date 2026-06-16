#version 460

layout(constant_id = 0) const bool ENABLE_CUTOUT = false;
layout(constant_id = 1) const bool ENABLE_DEBUG = false;

#include "Lighting.glsl"

layout(early_fragment_tests) in;
void main()
{
    LightingInput lightingInput = CreateLightingInput(vMaterialIndex);

    if(ENABLE_CUTOUT && lightingInput.Alpha < 0.5) discard;

    outColor = vec4(ComputeLighting(lightingInput), 1.0);
}
