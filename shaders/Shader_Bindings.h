#ifndef SHADER_BINDINGS_INCLUDE
#define SHADER_BINDINGS_INCLUDE


#define GLOBAL_SET 0

#define BINDING_TEXTURES 0

#define BINDING_CAMERA_BUFFER (BINDING_TEXTURES + 1)
#define STATIC_BINDING_MATERIAL_BUFFER (BINDING_CAMERA_BUFFER + 1)
#define STATIC_BINDING_INSTANCE_DATA_BUFFER (STATIC_BINDING_MATERIAL_BUFFER + 1)

//if GLSL define a type dUint for uint
// if cpu define a type dUint for uint32_t

#ifdef GLSL
#define dUint uint
#else
#include <cstdint>
typedef uint32_t dUint;
#endif

#ifdef GLSL
layout(push_constant) uniform PushConstants
#else
struct PushConstants
#endif
{
    dUint _CameraBufferIndex;
#ifdef GLSL
} push;
#else
};
#endif

#endif
