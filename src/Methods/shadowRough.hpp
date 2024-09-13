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
        FRAGMENT/GENERIC SHADING
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

        p_genericShaderMethod =
            dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
                .tasks = {p_shadowLightFactor}, .friendlyName = "ShadowRough"});

        /*
        COMPOSING
        */
        auto p_shadowClear = root.getComponent<ComposeClearRenderKeeper>().new_asset(
            {ComposeClearRender::SetupParams{.root = root,
                                             .backgroundColor = glm::vec4(0.0f, 0.5f, 0.8f, 1.0f),
                                             .displayOutputPropertyName = "shadow_cleared"}});

        auto p_shadowRender = root.getComponent<ComposeSceneRenderKeeper>().new_asset(
            {ComposeSceneRender::SetupParams{.root = root,
                                             .sceneInputPropertyName = "scene",
                                             .vertexPositionOutputPropertyName =
                                                 "position_shadow_view",
                                             .displayInputPropertyName = "shadow_cleared",
                                             .displayOutputPropertyName = "rendered_shadow",
                                             .cullingMode = CullingMode::Frontface}});

        auto p_shadowTexture = dynasma::makeStandalone<ComposeFrameToTexture>(
            ComposeFrameToTexture::SetupParams{.root = root,
                                               .frameInputPropertyName = "rendered_shadow",
                                               .colorTextureOutputPropertyName = "",
                                               .depthTextureOutputPropertyName = "tex_shadow",
                                               .size = String("ShadowMapSize"),
                                               .horWrap = Texture::WrappingType::BORDER_COLOR,
                                               .verWrap = Texture::WrappingType::BORDER_COLOR,
                                               .minFilter = Texture::FilterType::NEAREST,
                                               .magFilter = Texture::FilterType::NEAREST,
                                               .useMipMaps = false});

        // compose method
        p_composeMethod =
            dynasma::makeStandalone<Method<ComposeTask>>(Method<ComposeTask>::MethodParams{
                .tasks = {p_shadowClear, p_shadowRender, p_shadowTexture},
                .friendlyName = "ShadowRough"});
    }
};