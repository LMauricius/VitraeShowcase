#pragma once

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Compositing/AdaptTasks.hpp"
#include "Vitrae/Pipelines/Compositing/Function.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"

#include "dynasma/standalone.hpp"

namespace VitraeCommon
{
    using namespace Vitrae;

    inline void setupDisplayCamera(ComponentRoot &root)
    {
        MethodCollection &methodCollection = root.getComponent<MethodCollection>();

        // shadow matrices extractor
        auto p_extractLightProperties =
            dynasma::makeStandalone<ComposeFunction>(
                ComposeFunction::SetupParams{
                    .inputSpecs = {{
                        ParamSpec{.name = "scene",
                                  .typeInfo = TYPE_INFO<dynasma::FirmPtr<Scene>>},
                        ParamSpec{.name = "fs_shadow",
                                  .typeInfo = TYPE_INFO<dynasma::FirmPtr<FrameStore>>},
                    }},
                    .outputSpecs = {{
                        ParamSpec{.name = "mat_shadow_view",
                                  .typeInfo = TYPE_INFO<glm::mat4>},
                        ParamSpec{.name = "mat_shadow_persp",
                                  .typeInfo = TYPE_INFO<glm::mat4>},
                        ParamSpec{.name = "light_direction",
                                  .typeInfo = TYPE_INFO<glm::vec3>},
                        ParamSpec{.name = "light_color_primary",
                                  .typeInfo = TYPE_INFO<glm::vec3>},
                        ParamSpec{.name = "light_color_ambient",
                                  .typeInfo = TYPE_INFO<glm::vec3>},
                    }},
                    .p_function =
                        [](const RenderComposeContext &context)
                    {
                        try
                        {
                            auto p_scene =
                                context.properties.get("scene").get<dynasma::FirmPtr<Scene>>();
                            auto p_shadowFrame =
                                context.properties.get("fs_shadow").get<dynasma::FirmPtr<FrameStore>>();
                            context.properties.set(
                                "mat_shadow_view",
                                p_scene->light.getViewMatrix(p_scene->camera,
                                                             1.0 / p_shadowFrame->getSize().x));
                            context.properties.set("mat_shadow_persp",
                                                   p_scene->light.getProjectionMatrix());
                            context.properties.set("light_direction",
                                                   glm::normalize(p_scene->light.direction));
                            context.properties.set("light_color_primary",
                                                   p_scene->light.color_primary);
                            context.properties.set("light_color_ambient",
                                                   p_scene->light.color_ambient);
                        }
                        catch (const std::out_of_range &e)
                        {
                        }
                    },
                    .friendlyName = "Light properties",
                });
        methodCollection.registerComposeTask(p_extractLightProperties);

        // camera matrices extractor
        auto p_extractCameraProperties =
            dynasma::makeStandalone<ComposeFunction>(ComposeFunction::SetupParams{
                .inputSpecs = {{
                    ParamSpec{
                        .name = "scene", .typeInfo = TYPE_INFO<dynasma::FirmPtr<Scene>>},
                }},
                .outputSpecs = {{
                    ParamSpec{.name = "mat_camera_view",
                              .typeInfo = TYPE_INFO<glm::mat4>},
                    ParamSpec{.name = "camera_position",
                              .typeInfo = TYPE_INFO<glm::vec3>},
                }},
                .p_function =
                    [](const RenderComposeContext &context)
                {
                    try
                    {
                        auto p_scene =
                            context.properties.get("scene").get<dynasma::FirmPtr<Scene>>();

                        context.properties.set("mat_camera_view",
                                               p_scene->camera.getViewMatrix());
                        context.properties.set("camera_position", p_scene->camera.position);
                    }
                    catch (const std::out_of_range &e)
                    {
                        throw;
                    }
                },
                .friendlyName = "Camera properties",
            });
        methodCollection.registerComposeTask(p_extractCameraProperties);

        // camera matrices extractor
        auto p_extractCameraProjection =
            dynasma::makeStandalone<ComposeFunction>(ComposeFunction::SetupParams{
                .inputSpecs = {{
                    ParamSpec{
                        .name = "scene", .typeInfo = TYPE_INFO<dynasma::FirmPtr<Scene>>},
                    ParamSpec{
                        .name = "fs_target", .typeInfo = TYPE_INFO<dynasma::FirmPtr<FrameStore>>},
                }},
                .outputSpecs = {{
                    ParamSpec{.name = "mat_camera_proj",
                              .typeInfo = TYPE_INFO<glm::mat4>},
                }},
                .p_function =
                    [](const RenderComposeContext &context)
                {
                    try
                    {
                        auto p_scene =
                            context.properties.get("scene").get<dynasma::FirmPtr<Scene>>();
                        auto p_windowFrame =
                            context.properties.get("fs_target")
                                .get<dynasma::FirmPtr<FrameStore>>();

                        context.properties.set(
                            "mat_camera_proj",
                            p_scene->camera.getPerspectiveMatrix(p_windowFrame->getSize().x,
                                                                 p_windowFrame->getSize().y));
                    }
                    catch (const std::out_of_range &e)
                    {
                        throw;
                    }
                },
                .friendlyName = "Camera projection",
            });
        methodCollection.registerComposeTask(p_extractCameraProjection);

        auto p_renderAdaptor = dynasma::makeStandalone<ComposeAdaptTasks>(
            ComposeAdaptTasks::SetupParams{.root = root,
                                           .adaptorAliases = {
                                               {"camera_displayed", "scene_rendered"},
                                               {"position_view", "position_camera_view"},
                                               {"fs_target", "fs_display"},
                                           },
                                           .desiredOutputs = {ParamSpec{
                                               "camera_displayed",
                                               TYPE_INFO<void>,
                                           }},
                                           .friendlyName = "Render camera"});
        methodCollection.registerComposeTask(p_renderAdaptor);

        methodCollection.registerCompositorOutput("camera_displayed");
    }
}