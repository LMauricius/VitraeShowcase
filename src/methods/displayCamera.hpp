#pragma once

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Compositing/AdaptTasks.hpp"
#include "Vitrae/Pipelines/Compositing/Function.hpp"
#include "Vitrae/ComponentRoot.hpp"
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
                        PropertySpec{.name = "scene",
                                     .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<Scene>>()},
                        PropertySpec{.name = "fs_shadow",
                                     .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<Scene>>()},
                    }},
                    .outputSpecs = {{
                        PropertySpec{.name = "mat_shadow_view",
                                     .typeInfo = Variant::getTypeInfo<glm::mat4>()},
                        PropertySpec{.name = "mat_shadow_persp",
                                     .typeInfo = Variant::getTypeInfo<glm::mat4>()},
                        PropertySpec{.name = "light_direction",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                        PropertySpec{.name = "light_color_primary",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                        PropertySpec{.name = "light_color_ambient",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
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

        // camera matrices extractor
        auto p_extractCameraProperties =
            dynasma::makeStandalone<ComposeFunction>(ComposeFunction::SetupParams{
                .inputSpecs = {{
                    PropertySpec{
                        .name = "scene", .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<Scene>>()},
                    PropertySpec{
                        .name = "fs_target", .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>()},
                }},
                .outputSpecs = {{
                    PropertySpec{.name = "mat_camera_view",
                                 .typeInfo = Variant::getTypeInfo<glm::mat4>()},
                    PropertySpec{.name = "mat_camera_proj",
                                 .typeInfo = Variant::getTypeInfo<glm::mat4>()},
                    PropertySpec{.name = "camera_position",
                                 .typeInfo = Variant::getTypeInfo<glm::vec3>()},
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

                        context.properties.set("mat_camera_view",
                                               p_scene->camera.getViewMatrix());
                        context.properties.set(
                            "mat_camera_proj",
                            p_scene->camera.getPerspectiveMatrix(p_windowFrame->getSize().x,
                                                                 p_windowFrame->getSize().y));
                        context.properties.set("camera_position", p_scene->camera.position);
                    }
                    catch (const std::out_of_range &e)
                    {
                    }
                },
                .friendlyName = "Camera properties",
            });

        auto p_renderAdaptor = dynasma::makeStandalone<ComposeAdaptTasks>(
            ComposeAdaptTasks::SetupParams{.root = root,
                                           .adaptorAliases = {
                                               {"camera_displayed", "scene_rendered"},
                                               {"position_view", "position_camera_view"},
                                           },
                                           .desiredOutputs = {PropertySpec{
                                               "camera_displayed",
                                               Variant::getTypeInfo<void>(),
                                           }},
                                           .friendlyName = "Render shadows"});

        methodCollection.registerCompositorOutput("camera_displayed");
        methodCollection.registerComposeTask(p_extractLightProperties);
        methodCollection.registerComposeTask(p_extractCameraProperties);
    }
}