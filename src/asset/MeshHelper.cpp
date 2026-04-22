#include "asset/MeshHelper.h"
#include "asset/Vertex.h"
#include "registry/MeshRegistry.h"

SubMesh MeshHelper::CubeMesh(MeshRegistry& meshRegistry)
{
    std::array<Vertex, 8> vertices
    {
        Vertex
        {
            .pos = {-0.5f, -0.5f, -0.5f},
            .texCoord = {0.0f, 0.0f}
        },
        Vertex
        {
            .pos = {0.5f, -0.5f, -0.5f},
            .texCoord = {1.0f, 0.0f}
        },
        Vertex
        {
            .pos = {0.5f, -0.5f, 0.5f},
            .texCoord = {1.0f, 1.0f}
        },
        Vertex
        {
            .pos = {-0.5f, -0.5f, 0.5f},
            .texCoord = {0.0f, 1.0f}
        },
        Vertex
        {
            .pos = {-0.5f, 0.5f, -0.5f},
            .texCoord = {0.0f, 0.0f}
        },
        Vertex
        {
            .pos = {0.5f, 0.5f, -0.5f},
            .texCoord = {1.0f, 0.0f}
        },
        Vertex
        {
            .pos = {0.5f, 0.5f, 0.5f},
            .texCoord = {1.0f, 1.0f}
        },
        Vertex
        {
            .pos = {-0.5f, 0.5f, 0.5f},
            .texCoord = {0.0f, 1.0f}
        }
    };

    std::array<uint32_t, 36> indices
    {
        0, 1, 2, 2, 3, 0,
        1, 5, 6, 6, 2, 1,
        5, 4, 7, 7, 6, 5,
        4, 0, 3, 3, 7, 4,
        4, 5, 1, 1, 0, 4,
        6, 7, 3, 3, 2, 6
    };

    return meshRegistry.Register(vertices, indices);
}
