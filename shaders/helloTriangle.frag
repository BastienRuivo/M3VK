#version 450

//inputs
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoords;

layout(binding = 1) uniform sampler2D texSampler;

// output
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(texSampler, fragTexCoords);
    outColor.rgb = (fragColor + outColor.rgb) * 0.5;
    //outColor = vec4(vec3(gl_FragCoord.z * 400), 1.0);
}
