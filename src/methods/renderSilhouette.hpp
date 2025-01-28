#pragma once

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"

#include "dynasma/standalone.hpp"

namespace VitraeCommon
{
    using namespace Vitrae;

    inline void setupRenderSilhouette(ComponentRoot &root)
    {
        MethodCollection &methodCollection = root.getComponent<MethodCollection>();

        auto p_clear = root.getComponent<ComposeClearRenderKeeper>().new_asset(
            {ComposeClearRender::SetupParams{.root = root,
                                             .backgroundColor = glm::vec4(0.0f, 0.5f, 0.8f, 1.0f),
                                             .outputTokenNames = {"frame_cleared"}}});
        methodCollection.registerComposeTask(p_clear);

        auto p_forwardRender = root.getComponent<ComposeSceneRenderKeeper>().new_asset(
            {ComposeSceneRender::SetupParams{
                .root = root,
                .inputTokenNames = {"frame_cleared"},
                .outputTokenNames = {"scene_silhouette_rendered"},

                .rasterizing =
                    {
                        .vertexPositionOutputPropertyName = "position_view",
                        .modelFormPurpose = Purposes::silhouetting,
                        .cullingMode = CullingMode::Frontface,
                    },
                .ordering =
                    {
                        .generateFilterAndSort = [](const Scene &scene,
                                                    const RenderComposeContext &ctx)
                            -> std::pair<ComposeSceneRender::FilterFunc,
                                         ComposeSceneRender::SortFunc> {
                            glm::vec3 camPos = scene.camera.position;

                            return {
                                [](const ModelProp &prop) { return true; },
                                [camPos](const ModelProp &l, const ModelProp &r) {
                                    // closest first
                                    glm::vec3 lpos = l.transform.position;
                                    glm::vec3 rpos = r.transform.position;
                                    return glm::length2(camPos - rpos) <
                                           glm::length2(camPos - lpos);
                                },
                            };
                        },
                    },
            }});
        methodCollection.registerComposeTask(p_forwardRender);

        methodCollection.registerPropertyOption("scene_rendered", "scene_silhouette_rendered");
    }
}