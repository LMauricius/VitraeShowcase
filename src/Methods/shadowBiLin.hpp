#pragma once

#include "../shadingModes.hpp"
#include "abstract.hpp"

#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/FrameToTexture.hpp"
#include "Vitrae/Pipelines/Compositing/Function.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Shading/Constant.hpp"
#include "Vitrae/Pipelines/Shading/Snippet.hpp"

#include "dynasma/standalone.hpp"

#include <iostream>

using namespace Vitrae;

// "Edge Bilinear Range"
struct MethodsShadowBiLin : MethodCollection
{
    inline MethodsShadowBiLin(ComponentRoot &root) : MethodCollection(root)
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
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            PropertySpec{.name = "tex_shadow",
                                         .typeInfo = Variant::getTypeInfo<
                                             dynasma::FirmPtr<Texture>>()},
                            PropertySpec{.name = "position_shadow",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                        },
                    .outputSpecs =
                        {
                            PropertySpec{.name = "light_shadow_factor",
                                         .typeInfo =
                                             Variant::getTypeInfo<float>()},
                        },
                    .snippet = R"(
                        vec2 shadowSize = textureSize(tex_shadow, 0);
                        float offset = 0.5 / shadowSize.x;
                        vec2 texelPos = position_shadow.xy * shadowSize;
                        vec2 bilinOffset = texelPos - floor(texelPos);

                        bvec4 inShadow = lessThan(
                            textureGather(tex_shadow, position_shadow.xy - offset, 0),
                            vec4(position_shadow.z + offset)
                        );

                        light_shadow_factor = (
                            (inShadow.w? 0.0 : 1.0 - bilinOffset.x) + (inShadow.z? 0.0 : bilinOffset.x)
                        ) * (1.0 - bilinOffset.y) + (
                            (inShadow.x? 0.0 : 1.0 - bilinOffset.x) + (inShadow.y? 0.0 : bilinOffset.x)
                        ) * bilinOffset.y;
                )"}});

        p_genericShaderMethod =
            dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
                .tasks = {p_shadowLightFactor}, .friendlyName = "ShadowBiLin"});

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
                .friendlyName = "ShadowBiLin"});
    }
};