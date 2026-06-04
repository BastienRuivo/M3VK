#version 460

#define ALPHA_CUTOUT 1

#include "Lighting.glsl"

void main()
{
    LightingInput lightingInput = CreateLightingInput(vMaterialIndex);

    if (lightingInput.Alpha < 0.5)
    {
        discard;
    }

    outColor = vec4(ComputeLighting(lightingInput), 1.0);
}
