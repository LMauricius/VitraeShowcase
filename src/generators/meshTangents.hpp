#pragma once

#include "../Standard/Params.hpp"

#include "Vitrae/Params/Standard.hpp"
#include "Vitrae/Assets/Shapes/Mesh.hpp"
#include "Vitrae/Assets/BufferUtil/SubPtr.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MeshGenerator.hpp"

#include "dynasma/standalone.hpp"

namespace VitraeCommon
{
    using namespace Vitrae;

    inline StableMap<StringId, SharedSubBufferVariantPtr> generateMeshTangents(ComponentRoot &root, Mesh &mesh)
    {
        // ensure the dependency components are loaded
        mesh.prepareComponents(ParamList{::StandardParam::position, ::StandardParam::normal, ::StandardParam::texcoord0});

        // Extract existing data
        std::span<const Triangle> triangles = mesh.getTriangles();
        StridedSpan<const glm::vec3>
            positions = mesh.getVertexComponentData<glm::vec3>(::StandardParam::position.name);
        StridedSpan<const glm::vec3>
            normals = mesh.getVertexComponentData<glm::vec3>(::StandardParam::normal.name);
        StridedSpan<const glm::vec2>
            texcoords = mesh.getVertexComponentData<glm::vec2>(::StandardParam::texcoord0.name);

        // Create buffers
        auto [p_tangentBuf, p_bitangentBuf] = makeBufferInterleaved<glm::vec3, glm::vec3>(
            root, BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW, positions.size(), "tangent+bitangent arrays");

        StridedSpan<glm::vec3> tangents = p_tangentBuf.getMutableElements();
        StridedSpan<glm::vec3> bitangents = p_bitangentBuf.getMutableElements();

        // initialize to 0
        for (size_t i = 0; i < tangents.size(); ++i)
        {
            tangents[i] = {0.0f, 0.0f, 0.0f};
            bitangents[i] = {0.0f, 0.0f, 0.0f};
        }

        // Calculate tangents and bitangents
        for (const Triangle &tri : triangles)
        {
            std::size_t i0 = tri.ind[0];
            std::size_t i1 = tri.ind[1];
            std::size_t i2 = tri.ind[2];

            const glm::vec3 &p0 = positions[i0];
            const glm::vec3 &p1 = positions[i1];
            const glm::vec3 &p2 = positions[i2];

            const glm::vec2 &uv0 = texcoords[i0];
            const glm::vec2 &uv1 = texcoords[i1];
            const glm::vec2 &uv2 = texcoords[i2];

            glm::vec3 edge1 = p1 - p0;
            glm::vec3 edge2 = p2 - p0;

            glm::vec2 deltaUV1 = uv1 - uv0;
            glm::vec2 deltaUV2 = uv2 - uv0;

            float det = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;
            if (std::abs(det) < 1e-6f)
                continue; // Prevent division by zero

            float invDet = 1.0f / det;

            glm::vec3 tangent = (edge1 * deltaUV2.y - edge2 * deltaUV1.y) * invDet;
            glm::vec3 bitangent = (edge2 * deltaUV1.x - edge1 * deltaUV2.x) * invDet;

            // Compute triangle surface area
            float area2 = glm::length(glm::cross(edge1, edge2));

            // Accumulate weighted contributions for each vertex
            tangents[i0] += tangent * area2;
            tangents[i1] += tangent * area2;
            tangents[i2] += tangent * area2;

            bitangents[i0] += bitangent * area2;
            bitangents[i1] += bitangent * area2;
            bitangents[i2] += bitangent * area2;
        }

        // Divide by total accumulated weight (just normalize lol)
        for (size_t i = 0; i < tangents.size(); ++i)
        {
            tangents[i] = glm::normalize(tangents[i]);
            bitangents[i] = glm::normalize(bitangents[i]);
        }

        // Return the buffers we want to add
        return {
            {StandardParam::tangent.name, p_tangentBuf},
            {StandardParam::bitangent.name, p_bitangentBuf},
        };
    }

    inline void setupMeshTangentGenerator(ComponentRoot &root)
    {
        MeshGeneratorCollection &meshGenerators = root.getComponent<MeshGeneratorCollection>();

        meshGenerators.registerGeneratorForComponents({{StandardParam::tangent.name, StandardParam::bitangent.name}}, generateMeshTangents);
    }
}