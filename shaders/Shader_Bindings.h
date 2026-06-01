#ifndef SHADER_BINDINGS_INCLUDE
#define SHADER_BINDINGS_INCLUDE


#define GLOBAL_SET 0

#define BINDING_TEXTURES 0

#define BINDING_CAMERA_BUFFER (BINDING_TEXTURES + 1)
#define STATIC_BINDING_MATERIAL_BUFFER (BINDING_CAMERA_BUFFER + 1)
#define STATIC_BINDING_INSTANCE_DATA_BUFFER (STATIC_BINDING_MATERIAL_BUFFER + 1)

// CUlling shader
#define STATIC_BINDING_DRAW_INDIRECT_BUFFER (STATIC_BINDING_INSTANCE_DATA_BUFFER + 1)
#define BINDING_VISIBLE_INSTANCE_DATA_BUFFER (STATIC_BINDING_DRAW_INDIRECT_BUFFER + 1)
#define BINDING_VISIBLE_DRAW_INDIRECT_BUFFER (BINDING_VISIBLE_INSTANCE_DATA_BUFFER + 1)

//if GLSL define a type dUint for uint
// if cpu define a type dUint for uint32_t

#ifdef GLSL
#define dUint uint
#else
#include <cstdint>
typedef uint32_t dUint;
#endif

// BufferIndexes only contains index for non static buffers
// static buffers are bound at their creation
// but other buffers are stored in an array, and we send the index within the array via push constants
// so we can retrieve the buffer we want without binding another descriptor

// like here, we have a camera buffer per frame, so we use an array _Cameras[] that link to the camera buffer via index
// layout(set = GLOBAL_SET, binding = BINDING_CAMERA_BUFFER, std430) readonly buffer CameraDataBuffer
// {
//     CameraData data;
// } _Cameras[];

struct BufferIndexes
{
    dUint Cameras;
    dUint VisibleInstanceDatas;
    dUint VisibleDrawIndirects;
};

#endif
