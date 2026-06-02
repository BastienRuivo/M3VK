#ifndef CAMERA_INCLUDE
#define CAMERA_INCLUDE

#include "dTypes.h"

struct CameraData
{
    dMat4 WorldToCameraMatrix;
    dMat4 ProjectionMatrix;
    dMat4 ViewProjectionMatrix;
};

#endif
