#version 460

#include "Lighting.glsl"

void main()
{
    LightingInput lightingInput = CreateLightingInput(vMaterialIndex);
    outColor = vec4(ComputeLighting(lightingInput), 1.0);
}
