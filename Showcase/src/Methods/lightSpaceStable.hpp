#pragma once

#include "../shadingModes.hpp"
#include "abstract.hpp"

#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/FrameToTexture.hpp"
#include "Vitrae/Pipelines/Compositing/Function.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Shading/Constant.hpp"
#include "Vitrae/Pipelines/Shading/Function.hpp"

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
            root.getComponent<ShaderFunctionKeeper>().new_asset({ShaderFunction::StringParams{
                .inputSpecs =
                    {
                        PropertySpec{.name = "position_world",
                                     .typeInfo = Variant::getTypeInfo<glm::vec4>()},
                        PropertySpec{.name = "mat_shadow_view",
                                     .typeInfo = Variant::getTypeInfo<glm::mat4>()},
                        PropertySpec{.name = "mat_shadow_persp",
                                     .typeInfo = Variant::getTypeInfo<glm::mat4>()},
                    },
                .outputSpecs =
                    {
                        PropertySpec{.name = "position_shadow",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                        PropertySpec{.name = "position_shadow_view",
                                     .typeInfo = Variant::getTypeInfo<glm::vec4>()},
                    },
                .snippet = R"(
                    void shadowPosition(
                        in vec4 position_world, in mat4 mat_shadow_view, in mat4 mat_shadow_persp,
                        out vec3 position_shadow, out vec4 position_shadow_view
                    ) {
                        position_shadow_view = mat_shadow_persp * mat_shadow_view * position_world;
                        position_shadow = position_shadow_view.xyz / position_shadow_view.w * 0.5 + 0.5;
                    }
                )",
                .functionName = "shadowPosition"}});

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
            dynasma::makeStandalone<ComposeFunction>(ComposeFunction::SetupParams{
                .inputSpecs = {{
                    PropertySpec{.name = "scene",
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
                .p_function = [](const RenderRunContext &context) {
                    try {
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
                        context.properties.set("light_color_primary", p_scene->light.color_primary);
                        context.properties.set("light_color_ambient", p_scene->light.color_ambient);
                    }
                    catch (const std::out_of_range &e) {
                    }
                }});

        // compose method
        p_composeMethod =
            dynasma::makeStandalone<Method<ComposeTask>>(Method<ComposeTask>::MethodParams{
                .tasks = {p_extractLightProperties}, .friendlyName = "LSStable"});
    }
};