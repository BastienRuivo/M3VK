#version 450

// Note: Vec3 & stuff uses multiple location space see https://wikis.khronos.org/opengl/Layout_Qualifier_(GLSL)
// In
layout(location = 0) in vec2 osVertexPosition;
layout(location = 1) in vec3 vertexColor;

// Out
layout(location=0) out vec3 fragColor;

void main()
{
    gl_Position = vec4(osVertexPosition, 0.0, 1.0);
    fragColor = vertexColor;
}
