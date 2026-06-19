#version 460

#include "../header/GlobalIncludes.glsl"

layout(push_constant) uniform PushConstants
{
    CommonIndexes indexes;
} push;

layout(location = 0) in vec3 vViewDir;
layout(location = 0) out vec4 outColor;

layout(set = GLOBAL_SET, binding = STATIC_BINDING_SKYBOX_TEXTURE) uniform samplerCube skyboxSampler;

//layout(early_fragment_tests) in;
void main()
{
    outColor = texture(skyboxSampler, normalize(vViewDir));
}
