#ifndef DTYPES_INCLUDE
#define DTYPES_INCLUDE

#ifdef GLSL
    #define dAlign(x)
    #define dUint uint
    #define dInt int
    #define dMat4 mat4
    #define dMat3 mat3
    #define dVec4 vec4
    #define dVec3 vec3
    #define dVec2 vec2
#else
    #include <cstdint>
    #include <glm/glm.hpp>

    #define dAlign(x) alignas(x)
    typedef uint32_t dUint;
    typedef int32_t dInt;
    typedef glm::mat4 dMat4;
    typedef glm::mat3 dMat3;
    typedef glm::vec4 dVec4;
    typedef glm::vec3 dVec3;
    typedef glm::vec2 dVec2;
#endif

#endif
