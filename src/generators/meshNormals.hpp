#pragma once

#include "../Standard/Params.hpp"

#include "Vitrae/Params/Standard.hpp"
#include "Vitrae/Assets/Shapes/Mesh.hpp"
#include "Vitrae/Assets/BufferUtil/SubPtr.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MeshGenerator.hpp"
#include "Vitrae/Renderer.hpp"

#include "dynasma/standalone.hpp"

namespace VitraeCommon
{
    using namespace Vitrae;

    inline StableMap<StringId, SharedSubBufferVariantPtr> generateMeshNormals(ComponentRoot &root, Mesh &mesh)
    {
        // ensure the dependency components are loaded
        mesh.prepareComponents(ParamList{::StandardParam::position});

        // Extract existing data
        std::span<const Triangle> triangles = mesh.getTriangles();
        StridedSpan<const glm::vec3>
            positions = mesh.getVertexComponentData<glm::vec3>(::StandardParam::position.name);

        // Create buffers
        auto p_normalBuf = makeBuffer<void, glm::vec3>(
            root, BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW, positions.size(), "normal array");

        std::span<glm::vec3> normals = p_normalBuf.getMutableElements();

        // initialize to 0
        for (size_t i = 0; i < normals.size(); ++i)
        {
            normals[i] = {0.0f, 0.0f, 0.0f};
        }

        // Calculate normals
        for (const Triangle &tri : triangles)
        {
            std::size_t i0 = tri.ind[0];
            std::size_t i1 = tri.ind[1];
            std::size_t i2 = tri.ind[2];

            if (i0 >= positions.size() || i1 >= positions.size() || i2 >= positions.size()) {
                continue;
            }

            const glm::vec3 &p0 = positions[i0];
            const glm::vec3 &p1 = positions[i1];
            const glm::vec3 &p2 = positions[i2];

            // Get normal of the triangle
            glm::vec3 p01 = glm::normalize(p1 - p0);
            glm::vec3 p02 = glm::normalize(p2 - p0);
            glm::vec3 p12 = glm::normalize(p2 - p1);

            glm::vec3 edge1;
            glm::vec3 edge2;
            if (mesh.getFrontSideOrientation() == FrontSideOrientation::Clockwise)
            {
                edge1 = p12;
                edge2 = p01;
            }
            else
            {
                edge1 = p01;
                edge2 = p02;
            }

            glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

            // Weight according to angle of the triangle point
            float w0 = glm::acos(glm::dot(p01, p02));
            float w1 = glm::acos(glm::dot(p12, -p01));
            float w2 = glm::acos(glm::dot(p12, p02));

            normals[i0] += normal * w0;
            normals[i1] += normal * w1;
            normals[i2] += normal * w2;
        }

        // Divide by total accumulated weight (just normalize lol)
        for (size_t i = 0; i < normals.size(); ++i)
        {
            normals[i] = glm::normalize(normals[i]);
        }

        // Return the buffers we want to add
        return {
            {StandardParam::normal.name, p_normalBuf},
        };
    }

    inline void setupMeshNormalGenerator(ComponentRoot &root)
    {
        MeshGeneratorCollection &meshGenerators = root.getComponent<MeshGeneratorCollection>();
        Renderer &renderer = root.getComponent<Renderer>();

        meshGenerators.registerGeneratorForComponents({{StandardParam::normal.name}}, generateMeshNormals);
    }
}