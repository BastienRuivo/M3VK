#version 460

// KEYWORD ALPHA_CUTOUT
// KEYWORD DEBUG

#include "Lighting.glsl"

layout(early_fragment_tests) in;
void main()
{
    LightingInput lightingInput = CreateLightingInput(vMaterialIndex);

    #if defined(ALPHA_CUTOUT)
    {
        if (lightingInput.Alpha < 0.5)
        {
            discard;
        }
    }
    #endif

    outColor = vec4(ComputeLighting(lightingInput), 1.0);
}
