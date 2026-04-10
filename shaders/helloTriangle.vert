#version 450

// Note: Vec3 & stuff uses multiple location space see https://wikis.khronos.org/opengl/Layout_Qualifier_(GLSL)
// In
layout(location = 0) in vec3 osVertexPosition;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec2 texCoords;


// don't forget alignement the day you will have vec2 or nested
layout(set = 0, binding = 0) uniform CameraData
{
    mat4 worldToCameraMatrix;
    mat4 projectionMatrix;
    mat4 viewProjectionMatrix;
} _Camera;

layout(std430, binding = 1) readonly buffer ObjectData
{
    mat4 localToWorldMatrix;
} _Instance;

// Out
layout(location=0) out vec3 fragColor;
layout(location=1) out vec2 fragTexCoords;

void main()
{
    mat4 model = _Instance.localToWorldMatrix;
    gl_Position = _Camera.projectionMatrix * _Camera.worldToCameraMatrix * model * vec4(osVertexPosition, 1.0);
    fragColor = vertexColor;
    fragTexCoords = texCoords;
}
