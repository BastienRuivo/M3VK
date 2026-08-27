#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include <glm/gtx/hash.hpp>

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec4 tangent;
    glm::vec2 texCoord;

    bool operator==(const Vertex& other) const
    {
        return pos == other.pos
            && normal == other.normal
            && tangent == other.tangent
            && texCoord == other.texCoord;
    }

    static vk::VertexInputBindingDescription2EXT GetBindingDescription()
    {
        vk::VertexInputBindingDescription2EXT description{};
        description.binding = 0;
        description.stride = sizeof(Vertex);
        description.inputRate = vk::VertexInputRate::eVertex;
        description.divisor = 1;
        return description;
    }

    static vk::VertexInputAttributeDescription2EXT MakeAttribute(uint32_t location, vk::Format format, uint32_t offset)
    {
        vk::VertexInputAttributeDescription2EXT attribute{};
        attribute.location = location;
        attribute.binding = 0;
        attribute.format = format;
        attribute.offset = offset;
        return attribute;
    }

    static std::vector<vk::VertexInputAttributeDescription2EXT> GetAttributeDescription()
    {
        uint32_t location = 0;
        std::vector<vk::VertexInputAttributeDescription2EXT> attributeDescriptions
        {
            MakeAttribute(location++, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
            MakeAttribute(location++, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)),
            MakeAttribute(location++, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, tangent)),
            MakeAttribute(location++, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord))
        };

        return attributeDescriptions;
    }
};

namespace std {
    inline size_t HashCombine(uint32_t seed, uint32_t value)
    {
        return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            size_t seed = 0;

            HashCombine(seed, hash<glm::vec3>()(vertex.pos));
            HashCombine(seed, hash<glm::vec3>()(vertex.normal));
            HashCombine(seed, hash<glm::vec3>()(vertex.tangent));
            HashCombine(seed, hash<glm::vec2>()(vertex.texCoord));

            return seed;
        }
    };
}
