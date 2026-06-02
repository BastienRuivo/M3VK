#ifndef PROCEDURAL_INCLUDE
#define PROCEDURAL_INCLUDE

#include "dTypes.h"

struct DrawIndexedIndirectPadded {
    dUint    indexCount;
    dUint    instanceCount;
    dUint    firstIndex;
    dInt     vertexOffset;
    dUint    firstInstance;
    dUint pad0[3];
};

struct InstanceData
{
    dMat4 LocalToWorldMatrix;
    dVec3 AabbMin;
    dUint MaterialIndex;
    dVec3 AabbMax;
    dUint MeshIndex;
};

#endif
