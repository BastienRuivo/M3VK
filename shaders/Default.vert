#version 450

layout(push_constant, std430) uniform ObjectData
{
    mat4 localToWorldMatrix;
} _Instance;

// Note: Vec3 & stuff uses multiple location space see https://wikis.khronos.org/opengl/Layout_Qualifier_(GLSL)
// In
layout(location = 0) in vec3 osVertexPosition;
layout(location = 1) in vec2 texCoords;


// don't forget alignement the day you will have vec2 or nested
layout(set = 0, binding = 0) uniform CameraData
{
    mat4 WorldToCameraMatrix;
    mat4 ProjectionMatrix;
    mat4 ViewProjectionMatrix;
} _Camera;

// Out
layout(location=0) out vec2 fragTexCoords;

void main()
{
    mat4 model = _Instance.localToWorldMatrix;
    gl_Position = _Camera.ProjectionMatrix * _Camera.WorldToCameraMatrix * model * vec4(osVertexPosition, 1.0);
    fragTexCoords = texCoords;
}
