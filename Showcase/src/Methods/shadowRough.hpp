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

// "Edge Bilinear Range"
struct MethodsShadowRough : MethodCollection
{
    inline MethodsShadowRough(ComponentRoot &root) : MethodCollection(root)
    {
        /*
        VERTEX SHADING
        */
        p_vertexMethod = dynasma::makeStandalone<Method<ShaderTask>>(
            Method<ShaderTask>::MethodParams{.tasks = {}});

        /*
        FRAGMENT SHADING
        */

        auto p_shadowLightFactor =
            root.getComponent<ShaderFunctionKeeper>().new_asset({ShaderFunction::StringParams{
                .inputSpecs =
                    {
                        PropertySpec{.name = "tex_shadow",
                                     .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<Texture>>()},
                        PropertySpec{.name = "position_shadow",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                    },
                .outputSpecs =
                    {
                        PropertySpec{.name = "light_shadow_factor",
                                     .typeInfo = Variant::getTypeInfo<float>()},
                    },
                .snippet = R"(
                    void lightShadowFactor(
                        in sampler2D tex_shadow, in vec3 position_shadow,
                        out float light_shadow_factor)
                    {
                        vec2 shadowSize = textureSize(tex_shadow, 0);
                        ivec2 texelPosI = ivec2(round(position_shadow.xy * shadowSize));
                        float offset = 0.5 / shadowSize.x;
                        if (texelFetch(tex_shadow, texelPosI, 0).r < position_shadow.z + offset) {
                            light_shadow_factor = 0.0;
                        }
                        else {
                            light_shadow_factor = 1.0;
                        }
                    }
                )",
                .functionName = "lightShadowFactor"}});

        p_fragmentMethod =
            dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
                .tasks = {p_shadowLightFactor}, .friendlyName = "ShadowRough"});

        /*
        COMPOSING
        */

        // shadow matrices extractor
        auto p_extractLightProperties =
            dynasma::makeStandalone<ComposeFunction>(ComposeFunction::SetupParams{
                .inputSpecs = {{PropertySpec{
                    .name = "scene", .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<Scene>>()}}},
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
                    auto p_scene = context.properties.get("scene").get<dynasma::FirmPtr<Scene>>();
                    auto p_shadowFrame =
                        context.preparedCompositorFrameStores.at("rendered_shadow");
                    context.properties.set("mat_shadow_view",
                                           p_scene->light.getViewMatrix(
                                               p_scene->camera, 1.0 / p_shadowFrame->getSize().x));
                    context.properties.set("mat_shadow_persp",
                                           p_scene->light.getProjectionMatrix());
                    context.properties.set("light_direction",
                                           glm::normalize(p_scene->light.direction));
                    context.properties.set("light_color_primary", p_scene->light.color_primary);
                    context.properties.set("light_color_ambient", p_scene->light.color_ambient);
                }});

        // shadow render

        auto p_shadowClear = root.getComponent<ComposeClearRenderKeeper>().new_asset(
            {ComposeClearRender::SetupParams{.root = root,
                                             .backgroundColor = glm::vec4(0.0f, 0.5f, 0.8f, 1.0f),
                                             .displayOutputPropertyName = "shadow_cleared"}});

        auto p_shadowRender = root.getComponent<ComposeSceneRenderKeeper>().new_asset(
            {ComposeSceneRender::SetupParams{.root = root,
                                             .viewInputPropertyName = "mat_shadow_view",
                                             .perspectiveInputPropertyName = "mat_shadow_persp",
                                             .displayInputPropertyName = "shadow_cleared",
                                             .displayOutputPropertyName = "rendered_shadow",
                                             .cullingMode =
                                                 ComposeSceneRender::CullingMode::Frontface}});

        // compose method
        p_composeMethod =
            dynasma::makeStandalone<Method<ComposeTask>>(Method<ComposeTask>::MethodParams{
                .tasks = {p_extractLightProperties, p_shadowClear, p_shadowRender},
                .friendlyName = "ShadowEBR"});
    }
};