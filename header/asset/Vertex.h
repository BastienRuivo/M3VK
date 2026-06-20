#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

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

    static VkVertexInputBindingDescription2EXT GetBindingDescription()
    {
        VkVertexInputBindingDescription2EXT description
        {
            .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT,
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            .divisor = 1
        };
        return description;
    }

    static std::vector<VkVertexInputAttributeDescription2EXT> GetAttributeDescription()
    {
        uint32_t location = 0;
        std::vector<VkVertexInputAttributeDescription2EXT> attributeDescriptions
        {
            VkVertexInputAttributeDescription2EXT
            {
                .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
                .location = location++,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, pos)
            },
            VkVertexInputAttributeDescription2EXT
            {
                .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
                .location = location++,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, normal)
            },
            VkVertexInputAttributeDescription2EXT
            {
                .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
                .location = location++,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset = offsetof(Vertex, tangent)
            },
            VkVertexInputAttributeDescription2EXT
            {
                .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
                .location = location++,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, texCoord)
            }
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
