#ifndef CAMERA_INCLUDE
#define CAMERA_INCLUDE

#include "dTypes.h"

struct Plane
{
    dVec3 Normal;
    float Distance;

    #ifndef GLSL
    void Normalize()
    {
        float length = glm::length(Normal);
        Normal /= length;
        Distance /= length;
    }
    #endif
};

struct CameraData
{
    dMat4 WorldToCameraMatrix;
    dMat4 ProjectionMatrix;
    dMat4 ViewProjectionMatrix;
    dMat4 InverseViewProjectionMatrix;
    Plane FrustumPlanes[6];

    #ifndef GLSL
    enum Planes
    {
        Left = 0,
        Right = 1,
        Top = 2,
        Bottom = 3,
        Near = 4,
        Far = 5
    };
    #endif
};

#endif
