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

                .rasterizing = {
                    .vertexPositionOutputPropertyName = "position_view",
                    .modelFormPurpose = Purposes::visual,
                    .cullingMode = CullingMode::Frontface,
                },
                .ordering = {
                    .generateFilterAndSort = [](const Scene &scene, const RenderComposeContext &ctx) -> std::pair<ComposeSceneRender::FilterFunc, ComposeSceneRender::SortFunc>
                    {
                        return {
                            [](const ModelProp &prop)
                            {
                                return true;
                            },
                            [](const ModelProp &l, const ModelProp &r)
                            {
                                auto p_mat_l = l.p_model->getMaterial().getLoaded();
                                auto p_mat_r = r.p_model->getMaterial().getLoaded();
                                return p_mat_l->getParamAliases().hash() < p_mat_r->getParamAliases().hash() || p_mat_l < p_mat_r;
                            },
                        };
                    },
                },
            }});
        methodCollection.registerComposeTask(p_forwardRender);

        methodCollection.registerPropertyOption("scene_rendered", "scene_silhouette_rendered");
    }
}