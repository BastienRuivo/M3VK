#include "asset/MeshHelper.h"
#include "asset/Vertex.h"
#include "registry/MeshRegistry.h"
#include <cstdint>

uint32_t MeshHelper::CubeMesh(MeshRegistry& meshRegistry, glm::vec3& aabbMin, glm::vec3& aabbMax)
{
    std::array<Vertex, 24> vertices
    {
        // top
        Vertex
        {
            .pos = {-0.5f, 0.5f, -0.5f},
            .normal = {0.0f, 1.0f, 0.0f},
            .texCoord = {0.0f, 0.0f}
        },
        Vertex
        {
            .pos = {0.5f, 0.5f, -0.5f},
            .normal = {0.0f, 1.0f, 0.0f},
            .texCoord = {1.0f, 0.0f}
        },
        Vertex
        {
            .pos = {0.5f, 0.5f, 0.5f},
            .normal = {0.0f, 1.0f, 0.0f},
            .texCoord = {1.0f, 1.0f}
        },
        Vertex
        {
            .pos = {-0.5f, 0.5f, 0.5f},
            .normal = {0.0f, 1.0f, 0.0f},
            .texCoord = {0.0f, 1.0f}
        },
        // bottom
        Vertex
        {
            .pos = {-0.5f, -0.5f, -0.5f},
            .normal = {0.0f, -1.0f, 0.0f},
            .texCoord = {0.0f, 0.0f}
        },
        Vertex
        {
            .pos = {0.5f, -0.5f, -0.5f},
            .normal = {0.0f, -1.0f, 0.0f},
            .texCoord = {1.0f, 0.0f}
        },
        Vertex
        {
            .pos = {0.5f, -0.5f, 0.5f},
            .normal = {0.0f, -1.0f, 0.0f},
            .texCoord = {1.0f, 1.0f}
        },
        Vertex
        {
            .pos = {-0.5f, -0.5f, 0.5f},
            .normal = {0.0f, -1.0f, 0.0f},
            .texCoord = {0.0f, 1.0f}
        },
        // front
        Vertex
        {
            .pos = {-0.5f, 0.5f, 0.5f},
            .normal = {0.0f, 0.0f, 1.0f},
            .texCoord = {0.0f, 0.0f}
        },
        Vertex
        {
            .pos = {0.5f, 0.5f, 0.5f},
            .normal = {0.0f, 0.0f, 1.0f},
            .texCoord = {1.0f, 0.0f}
        },
        Vertex
        {
            .pos = {0.5f, -0.5f, 0.5f},
            .normal = {0.0f, 0.0f, 1.0f},
            .texCoord = {1.0f, 1.0f}
        },
        Vertex
        {
            .pos = {-0.5f, -0.5f, 0.5f},
            .normal = {0.0f, 0.0f, 1.0f},
            .texCoord = {0.0f, 1.0f}
        },
        // back
        Vertex
        {
            .pos = {-0.5f, 0.5f, -0.5f},
            .normal = {0.0f, 0.0f, -1.0f},
            .texCoord = {0.0f, 0.0f}
        },
        Vertex
        {
            .pos = {0.5f, 0.5f, -0.5f},
            .normal = {0.0f, 0.0f, -1.0f},
            .texCoord = {1.0f, 0.0f}
        },
        Vertex
        {
            .pos = {0.5f, -0.5f, -0.5f},
            .normal = {0.0f, 0.0f, -1.0f},
            .texCoord = {1.0f, 1.0f}
        },
        Vertex
        {
            .pos = {-0.5f, -0.5f, -0.5f},
            .normal = {0.0f, 0.0f, -1.0f},
            .texCoord = {0.0f, 1.0f}
        },
        // left
        Vertex
        {
            .pos = {-0.5f, 0.5f, -0.5f},
            .normal = {-1.0f, 0.0f, 0.0f},
            .texCoord = {0.0f, 0.0f}
        },
        Vertex
        {
            .pos = {-0.5f, 0.5f, 0.5f},
            .normal = {-1.0f, 0.0f, 0.0f},
            .texCoord = {1.0f, 0.0f}
        },
        Vertex
        {
            .pos = {-0.5f, -0.5f, 0.5f},
            .normal = {-1.0f, 0.0f, 0.0f},
            .texCoord = {1.0f, 1.0f}
        },
        Vertex
        {
            .pos = {-0.5f, -0.5f, -0.5f},
            .normal = {-1.0f, 0.0f, 0.0f},
            .texCoord = {0.0f, 1.0f}
        },
        // right
        Vertex
        {
            .pos = {0.5f, 0.5f, -0.5f},
            .normal = {1.0f, 0.0f, 0.0f},
            .texCoord = {0.0f, 0.0f}
        },
        Vertex
        {
            .pos = {0.5f, 0.5f, 0.5f},
            .normal = {1.0f, 0.0f, 0.0f},
            .texCoord = {1.0f, 0.0f}
        },
        Vertex
        {
            .pos = {0.5f, -0.5f, 0.5f},
            .normal = {1.0f, 0.0f, 0.0f},
            .texCoord = {1.0f, 1.0f}
        },
        Vertex
        {
            .pos = {0.5f, -0.5f, -0.5f},
            .normal = {1.0f, 0.0f, 0.0f},
            .texCoord = {0.0f, 1.0f}
        },
    };

    std::array<uint32_t, 36> indices
    {
        2, 1, 0, 0, 3, 2, // top
        4, 5, 6, 6, 7, 4, // bottom
        10, 9, 8, 8, 11, 10, // front
        12, 13, 14, 14, 15, 12, // back
        18, 17, 16, 16, 19, 18, // left
        20, 21, 22, 22, 23, 20 // right
    };

    aabbMin = { -0.5f, -0.5f, -0.5f };
    aabbMax = { 0.5f, 0.5f, 0.5f };

    return meshRegistry.RegisterMesh(vertices, indices);
}
