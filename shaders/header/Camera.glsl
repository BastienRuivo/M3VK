#ifndef CAMERA_INCLUDE
#define CAMERA_INCLUDE

struct CameraData
{
    mat4 WorldToCameraMatrix;
    mat4 ProjectionMatrix;
    mat4 ViewProjectionMatrix;
};

#endif
