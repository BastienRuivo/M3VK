#ifndef SHADER_BINDINGS_INCLUDE
#define SHADER_BINDINGS_INCLUDE


#define GLOBAL_SET 0

#define BINDING_TEXTURES 0

#define BINDING_CAMERA_BUFFER (BINDING_TEXTURES + 1)
#define BINDING_MATERIAL_BUFFER (BINDING_CAMERA_BUFFER + 1)
#define BINDING_INSTANCE_DATA_BUFFER (BINDING_MATERIAL_BUFFER + 1)

// CUlling shader
#define BINDING_CLEAR_DRAW_INDIRECT_BUFFER (BINDING_INSTANCE_DATA_BUFFER + 1)
#define BINDING_VISIBLE_INSTANCE_INDIRECTION_BUFFER (BINDING_CLEAR_DRAW_INDIRECT_BUFFER + 1)
#define BINDING_VISIBLE_DRAW_INDIRECT_BUFFER (BINDING_VISIBLE_INSTANCE_INDIRECTION_BUFFER + 1)

// Skybox
#define BINDING_SKYBOX_TEXTURE (BINDING_VISIBLE_DRAW_INDIRECT_BUFFER + 1)

// HiZ
#define BINDING_HIZ_TEXTURE  (BINDING_SKYBOX_TEXTURE + 1)

// BufferIndexes only contains index for non static buffers
// static buffers are bound at their creation
// but other buffers are stored in an array, and we send the index within the array via push constants
// so we can retrieve the buffer we want without binding another descriptor

// like here, we have a camera buffer per frame, so we use an array _Cameras[] that link to the camera buffer via index
// layout(set = GLOBAL_SET, binding = BINDING_CAMERA_BUFFER, std430) readonly buffer CameraDataBuffer
// {
//     CameraData data;
// } _Cameras[];

#include "dTypes.h"

struct CommonIndexes
{
    dUint DebugIndex;
    dUint Cameras;
    dUint VisibleInstanceIndirections;
    dUint VisibleDrawIndirects;
};
#endif
