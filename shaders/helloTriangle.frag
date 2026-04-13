#version 450

//inputs
layout(location = 0) in vec2 fragTexCoords;

layout(binding = 1) uniform sampler2D texSampler;

// output
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(texSampler, fragTexCoords);
}
