#ifndef INSTANCING_INCLUDE
#define INSTANCING_INCLUDE

struct ObjectData
{
    mat4 LocalToWorldMatrix;
    vec3 AabbMin;
    uint MaterialIndex;
    vec3 AabbMax;
    uint pad;
};

struct DrawIndexedIndirect
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int vertexOffset;
    uint firstInstance;
    uint pad0[3];
};

#endif
