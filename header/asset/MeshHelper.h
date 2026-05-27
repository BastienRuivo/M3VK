#pragma once

#include "glm/ext/vector_float3.hpp"
#include "registry/MeshRegistry.h"
#include <cstdint>
namespace MeshHelper
{
    uint32_t CubeMesh(MeshRegistry& meshRegistry, glm::vec3& aabbMin, glm::vec3& aabbMax);
};
