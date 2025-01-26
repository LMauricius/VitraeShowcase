#pragma once

#include "../Standard/Params.hpp"

#include "Vitrae/Assets/BufferUtil/SubPtr.hpp"
#include "Vitrae/Assets/Model.hpp"
#include "Vitrae/Assets/Shapes/Mesh.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/FormGenerator.hpp"
#include "Vitrae/Params/Purposes.hpp"
#include "Vitrae/Params/Standard.hpp"
#include "Vitrae/Renderer.hpp"

#include "dynasma/standalone.hpp"

namespace VitraeCommon
{

using namespace Vitrae;

DetailFormVector generateModelSilhouette(ComponentRoot &root, const Model &model)
{
    DetailFormSpan visualForms = model.getFormsWithPurpose(Purposes::visual);

    DetailFormVector silhouetteForms;
    silhouetteForms.reserve(visualForms.size());

    for (auto [p_LoDMeasure, p_shape] : visualForms) {
        auto p_loaded = p_shape.getLoaded();
        auto p_mesh = dynamic_cast<const Mesh *>(&*p_loaded);

        if (p_mesh) {

            auto positions =
                p_mesh->getVertexComponentData<glm::vec3>(StandardParam::position.name);

            // store unique positions
            struct PositionWOrigin
            {
                glm::vec3 position;
                std::uint32_t origin;
            };
            std::vector<PositionWOrigin> silEntries;
            silEntries.reserve(p_mesh->getTriangles().size() * 3);

            std::uint32_t sepInd = 0;
            for (const Triangle &tri : p_mesh->getTriangles()) {
                for (std::size_t i = 0; i < 3; ++i) {
                    silEntries.push_back(PositionWOrigin{positions[tri.ind[i]], sepInd});
                    ++sepInd;
                }
            }

            // remove multi-entries
            SharedBufferPtr<void, Triangle> buf_silIndices = makeBuffer<void, Triangle>(
                root, BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW, silEntries.size(),
                "silhouette index buffer");
            auto silIndices = (SharedBufferPtr<void, unsigned int>(buf_silIndices.getRawBuffer()))
                                  .getMutableElements();
            std::sort(silEntries.begin(), silEntries.end(),
                      [](const PositionWOrigin &a, const PositionWOrigin &b) {
                          return a.position.x < b.position.x ||
                                 (a.position.x == b.position.x &&
                                  (a.position.y < b.position.y ||
                                   (a.position.y == b.position.y && a.position.z < b.position.z)));
                      });
            std::uint32_t newInd = 0;
            silIndices[0] = 0;
            for (std::size_t entryI = 1; entryI < silEntries.size(); ++entryI) {
                if (silEntries[entryI].position != silEntries[newInd].position) {
                    ++newInd;
                    if (newInd != entryI) {
                        silEntries[newInd] = silEntries[entryI];
                    }
                }
                silIndices[silEntries[entryI].origin] = newInd;
            }

            auto buf_silPositions = makeBuffer<void, glm::vec3>(
                root, BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW, newInd,
                "silhouette position buffer");
            auto silPositions = (SharedBufferPtr<void, glm::vec3>(buf_silPositions.getRawBuffer()))
                                    .getMutableElements();
            for (std::size_t i = 0; i < newInd; ++i) {
                silPositions[i] = silEntries[i].position;
            }

            silhouetteForms.emplace_back(
                p_LoDMeasure,
                root.getComponent<MeshKeeper>().new_asset({Mesh::TriangleVerticesParams{
                    .root = root,
                    .vertexComponentBuffers = {{StandardParam::position.name, buf_silPositions}},
                    .indexBuffer = buf_silIndices,
                    .friendlyname = "silhouette"}}));
        }
    }

    return silhouetteForms;
}

inline void setupModelSilhouetteGenerator(ComponentRoot &root)
{
    root.getComponent<FormGeneratorCollection>()[Purposes::silhouetting] = generateModelSilhouette;
}

} // namespace VitraeCommon