#pragma once

#include "../shadingModes.hpp"
#include "abstract.hpp"

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/FrameToTexture.hpp"
#include "Vitrae/Pipelines/Compositing/Function.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Shading/Constant.hpp"
#include "Vitrae/Pipelines/Shading/Snippet.hpp"

#include "dynasma/standalone.hpp"

#include <iostream>

using namespace Vitrae;

// "Light space stable"
struct MethodsLSStable : MethodCollection
{
    inline MethodsLSStable(ComponentRoot &root) : MethodCollection(root)
    {
        /*
        VERTEX SHADING
        */
        auto p_shadowPosition =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            ParamSpec{.name = "position_world",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec4>},
                            ParamSpec{.name = "mat_shadow_view",
                                      .typeInfo =
                                          TYPE_INFO<glm::mat4>},
                            ParamSpec{.name = "mat_shadow_persp",
                                      .typeInfo =
                                          TYPE_INFO<glm::mat4>},
                        },
                    .outputSpecs =
                        {
                            ParamSpec{.name = "position_shadow",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec3>},
                            ParamSpec{.name = "position_shadow_view",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec4>},
                        },
                    .snippet = R"(
                        position_shadow_view = mat_shadow_persp * mat_shadow_view * position_world;
                        position_shadow = position_shadow_view.xyz / position_shadow_view.w * 0.5 + 0.5;
                )"}});

        p_vertexMethod =
            dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
                .tasks = {p_shadowPosition}, .friendlyName = "LSStable"});

        /*
        FRAGMENT SHADING
        */

        p_fragmentMethod = dynasma::makeStandalone<Method<ShaderTask>>(
            Method<ShaderTask>::MethodParams{.tasks = {}});

        /*
        COMPOSING
        */

        // shadow matrices extractor
        auto p_extractLightProperties =
            dynasma::makeStandalone<ComposeFunction>(
                ComposeFunction::SetupParams{
                    .inputSpecs = {{
                        ParamSpec{.name = "scene",
                                  .typeInfo = TYPE_INFO<dynasma::FirmPtr<Scene>>},
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
                                context.preparedCompositorFrameStores.at("rendered_shadow");
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

        // compose method
        p_composeMethod =
            dynasma::makeStandalone<Method<ComposeTask>>(Method<ComposeTask>::MethodParams{
                .tasks = {p_extractLightProperties}, .friendlyName = "LSStable"});
    }
};