#version 460

#include "Lighting.glsl"

layout(early_fragment_tests) in;
void main()
{
    LightingInput lightingInput = CreateLightingInput(vMaterialIndex);
    outColor = vec4(ComputeLighting(lightingInput), 1.0);
}
