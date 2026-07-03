#include "asset/MeshHelper.h"
#include "Material.h"
#include "asset/Vertex.h"
#include "allocation/MeshRegistry.h"
#include <cstdint>

uint32_t MeshHelper::CubeMesh(MeshRegistry& meshRegistry, MaterialType materialType, glm::vec3& aabbMin, glm::vec3& aabbMax)
{
    std::array<Vertex, 24> vertices
    {
        // top (Normal: 0, 1, 0) -> Tangent aligns with +X, UV matches standard top-down view
        Vertex { .pos = {-0.5f, 0.5f, -0.5f}, .normal = {0.0f, 1.0f, 0.0f}, .tangent = {1.0f, 0.0f, 0.0f, 1.0f}, .texCoord = {0.0f, 0.0f} },
        Vertex { .pos = {0.5f, 0.5f, -0.5f},  .normal = {0.0f, 1.0f, 0.0f}, .tangent = {1.0f, 0.0f, 0.0f, 1.0f}, .texCoord = {1.0f, 0.0f} },
        Vertex { .pos = {0.5f, 0.5f, 0.5f},   .normal = {0.0f, 1.0f, 0.0f}, .tangent = {1.0f, 0.0f, 0.0f, 1.0f}, .texCoord = {1.0f, 1.0f} },
        Vertex { .pos = {-0.5f, 0.5f, 0.5f},  .normal = {0.0f, 1.0f, 0.0f}, .tangent = {1.0f, 0.0f, 0.0f, 1.0f}, .texCoord = {0.0f, 1.0f} },

        // bottom (Normal: 0, -1, 0) -> Tangent aligns with +X
        Vertex { .pos = {-0.5f, -0.5f, 0.5f}, .normal = {0.0f, -1.0f, 0.0f}, .tangent = {1.0f, 0.0f, 0.0f, 1.0f}, .texCoord = {0.0f, 0.0f} },
        Vertex { .pos = {0.5f, -0.5f, 0.5f},  .normal = {0.0f, -1.0f, 0.0f}, .tangent = {1.0f, 0.0f, 0.0f, 1.0f}, .texCoord = {1.0f, 0.0f} },
        Vertex { .pos = {0.5f, -0.5f, -0.5f}, .normal = {0.0f, -1.0f, 0.0f}, .tangent = {1.0f, 0.0f, 0.0f, 1.0f}, .texCoord = {1.0f, 1.0f} },
        Vertex { .pos = {-0.5f, -0.5f, -0.5f},.normal = {0.0f, -1.0f, 0.0f}, .tangent = {1.0f, 0.0f, 0.0f, 1.0f}, .texCoord = {0.0f, 1.0f} },

        // front (Normal: 0, 0, 1) -> Tangent aligns with +X
        Vertex { .pos = {-0.5f, 0.5f, 0.5f},  .normal = {0.0f, 0.0f, 1.0f},  .tangent = {1.0f, 0.0f, 0.0f, 1.0f}, .texCoord = {0.0f, 0.0f} },
        Vertex { .pos = {0.5f, 0.5f, 0.5f},   .normal = {0.0f, 0.0f, 1.0f},  .tangent = {1.0f, 0.0f, 0.0f, 1.0f}, .texCoord = {1.0f, 0.0f} },
        Vertex { .pos = {0.5f, -0.5f, 0.5f},  .normal = {0.0f, 0.0f, 1.0f},  .tangent = {1.0f, 0.0f, 0.0f, 1.0f}, .texCoord = {1.0f, 1.0f} },
        Vertex { .pos = {-0.5f, -0.5f, 0.5f}, .normal = {0.0f, 0.0f, 1.0f},  .tangent = {1.0f, 0.0f, 0.0f, 1.0f}, .texCoord = {0.0f, 1.0f} },

        // back (Normal: 0, 0, -1) -> Tangent aligns with -X (due to looking from behind)
        Vertex { .pos = {0.5f, 0.5f, -0.5f},  .normal = {0.0f, 0.0f, -1.0f}, .tangent = {-1.0f, 0.0f, 0.0f, 1.0f}, .texCoord = {0.0f, 0.0f} },
        Vertex { .pos = {-0.5f, 0.5f, -0.5f}, .normal = {0.0f, 0.0f, -1.0f}, .tangent = {-1.0f, 0.0f, 0.0f, 1.0f}, .texCoord = {1.0f, 0.0f} },
        Vertex { .pos = {-0.5f, -0.5f, -0.5f},.normal = {0.0f, 0.0f, -1.0f}, .tangent = {-1.0f, 0.0f, 0.0f, 1.0f}, .texCoord = {1.0f, 1.0f} },
        Vertex { .pos = {0.5f, -0.5f, -0.5f}, .normal = {0.0f, 0.0f, -1.0f}, .tangent = {-1.0f, 0.0f, 0.0f, 1.0f}, .texCoord = {0.0f, 1.0f} },

        // left (Normal: -1, 0, 0) -> Tangent aligns with +Z
        Vertex { .pos = {-0.5f, 0.5f, -0.5f}, .normal = {-1.0f, 0.0f, 0.0f}, .tangent = {0.0f, 0.0f, 1.0f, 1.0f}, .texCoord = {0.0f, 0.0f} },
        Vertex { .pos = {-0.5f, 0.5f, 0.5f},  .normal = {-1.0f, 0.0f, 0.0f}, .tangent = {0.0f, 0.0f, 1.0f, 1.0f}, .texCoord = {1.0f, 0.0f} },
        Vertex { .pos = {-0.5f, -0.5f, 0.5f}, .normal = {-1.0f, 0.0f, 0.0f}, .tangent = {0.0f, 0.0f, 1.0f, 1.0f}, .texCoord = {1.0f, 1.0f} },
        Vertex { .pos = {-0.5f, -0.5f, -0.5f},.normal = {-1.0f, 0.0f, 0.0f}, .tangent = {0.0f, 0.0f, 1.0f, 1.0f}, .texCoord = {0.0f, 1.0f} },

        // right (Normal: 1, 0, 0) -> Tangent aligns with -Z
        Vertex { .pos = {0.5f, 0.5f, 0.5f},   .normal = {1.0f, 0.0f, 0.0f},  .tangent = {0.0f, 0.0f, -1.0f, 1.0f}, .texCoord = {0.0f, 0.0f} },
        Vertex { .pos = {0.5f, 0.5f, -0.5f},  .normal = {1.0f, 0.0f, 0.0f},  .tangent = {0.0f, 0.0f, -1.0f, 1.0f}, .texCoord = {1.0f, 0.0f} },
        Vertex { .pos = {0.5f, -0.5f, -0.5f}, .normal = {1.0f, 0.0f, 0.0f},  .tangent = {0.0f, 0.0f, -1.0f, 1.0f}, .texCoord = {1.0f, 1.0f} },
        Vertex { .pos = {0.5f, -0.5f, 0.5f},  .normal = {1.0f, 0.0f, 0.0f},  .tangent = {0.0f, 0.0f, -1.0f, 1.0f}, .texCoord = {0.0f, 1.0f} }
    };

    // Consolidated unified counter-clockwise winding layout for all faces
    std::array<uint32_t, 36> indices
    {
        0, 3, 2,  2, 1, 0, // top
        4, 7, 6,  6, 5, 4, // bottom
        8, 11, 10, 10, 9, 8, // front
        12, 15, 14, 14, 13, 12, // back
        16, 19, 18, 18, 17, 16, // left
        20, 23, 22, 22, 21, 20  // right
    };

    aabbMin = { -0.5f, -0.5f, -0.5f };
    aabbMax = { 0.5f, 0.5f, 0.5f };

    return meshRegistry.RegisterMesh(materialType, vertices, indices);
}
