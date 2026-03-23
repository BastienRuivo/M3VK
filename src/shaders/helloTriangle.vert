#version 450

// Note: Vec3 & stuff uses multiple location space see https://wikis.khronos.org/opengl/Layout_Qualifier_(GLSL)
// In
layout(location = 0) in vec3 osVertexPosition;
layout(location = 1) in vec3 vertexColor;


// don't forget alignement the day you will have vec2 or nested
layout(set = 0, binding = 0) uniform CameraData
{
    mat4 localToWorldMatrix;
    mat4 worldToCameraMatrix;
    mat4 projectionMatrix;
    mat4 viewProjectionMatrix;
} ubo;

// Out
layout(location=0) out vec3 fragColor;

void main()
{
    gl_Position = ubo.projectionMatrix * ubo.worldToCameraMatrix * ubo.localToWorldMatrix * vec4(osVertexPosition, 1.0);
    fragColor = vertexColor;
}
