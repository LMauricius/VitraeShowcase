#pragma once

#include "Vitrae/Assets/Mesh.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/Assets/Texture.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"

#include "dynasma/standalone.hpp"

#include "./helpers.hpp"

#include "glm/gtx/norm.hpp"

namespace VitraeCommon
{
    using namespace Vitrae;

    inline void setupRenderForward(ComponentRoot &root)
    {
        MethodCollection &methodCollection = root.getComponent<MethodCollection>();

        auto p_clear = root.getComponent<ComposeClearRenderKeeper>().new_asset(
            {ComposeClearRender::SetupParams{.root = root,
                                             .backgroundColor = glm::vec4(0.0f, 0.5f, 0.8f, 1.0f),
                                             .outputTokenNames = {"frame_cleared"}}});
        methodCollection.registerComposeTask(p_clear);

        auto p_forwardRenderOpaque = root.getComponent<ComposeSceneRenderKeeper>().new_asset(
            {ComposeSceneRender::SetupParams{
                .root = root,
                .inputTokenNames = {"frame_cleared"},
                .outputTokenNames = {"scene_forward_rendered_opaque"},
                .rasterizing = {
                    .vertexPositionOutputPropertyName = "position_view",
                    .rasterizingMode = RasterizingMode::DerivationalFillCenters,
                },
                .ordering = {
                    .generateFilterAndSort = [](const Scene &scene, const RenderComposeContext &ctx) -> std::pair<ComposeSceneRender::FilterFunc, ComposeSceneRender::SortFunc>
                    {
                        return {
                            [](const MeshProp &prop)
                            {
                                return isOpaque(*prop.p_mesh->getMaterial().getLoaded());
                            },
                            [](const MeshProp &l, const MeshProp &r)
                            {
                                auto p_mat_l = l.p_mesh->getMaterial().getLoaded();
                                auto p_mat_r = r.p_mesh->getMaterial().getLoaded();
                                return p_mat_l->getParamAliases().hash() < p_mat_r->getParamAliases().hash() || p_mat_l < p_mat_r;
                            },
                        };
                    },
                },
            }});
        methodCollection.registerComposeTask(p_forwardRenderOpaque);

        auto p_forwardRenderTransparent = root.getComponent<ComposeSceneRenderKeeper>().new_asset(
            {ComposeSceneRender::SetupParams{
                .root = root,
                .inputTokenNames = {"scene_forward_rendered_opaque"},
                .outputTokenNames = {"scene_forward_rendered"},
                .rasterizing = {
                    .vertexPositionOutputPropertyName = "position_view",
                    .sourceBlending = BlendingFunction::SourceAlpha,
                    .destinationBlending = BlendingFunction::OneMinusSourceAlpha,
                    .rasterizingMode = RasterizingMode::DerivationalFillCenters,
                    .writeDepth = false,
                },
                .ordering = {
                    .generateFilterAndSort = [](const Scene &scene, const RenderComposeContext &ctx) -> std::pair<ComposeSceneRender::FilterFunc, ComposeSceneRender::SortFunc>
                    {
                        glm::vec3 camPos = scene.camera.position;

                        return {
                            [](const MeshProp &prop)
                            {
                                return !isOpaque(*prop.p_mesh->getMaterial().getLoaded());
                            },
                            [camPos](const MeshProp &l, const MeshProp &r)
                            {
                                glm::vec3 lpos = l.transform.position;
                                glm::vec3 rpos = r.transform.position;
                                return glm::length2(camPos - rpos) < glm::length2(camPos - lpos);
                            },
                        };
                    },
                },
            }});
        methodCollection.registerComposeTask(p_forwardRenderTransparent);

        methodCollection.registerPropertyOption("scene_rendered", "scene_forward_rendered");
    }
}