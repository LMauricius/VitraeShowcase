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
struct MethodsShadowEBR : MethodCollection
{
    inline MethodsShadowEBR(ComponentRoot &root) : MethodCollection(root)
    {
        /*
        VERTEX SHADING
        */
        p_vertexMethod = dynasma::makeStandalone<Method<ShaderTask>>(
            Method<ShaderTask>::MethodParams{.tasks = {}});

        /*
        FRAGMENT/GENERIC SHADING
        */

        /*auto p_alias =
            root.getComponent<ShaderSnippetKeeper>().new_asset({ShaderSnippet::StringParams{
                .inputSpecs =
                    {
                        PropertySpec{.name = "position_shadow_view",
                                     .typeInfo =
           StandardShaderPropertyTypes::FRAGMENT_OUTPUT}, PropertySpec{.name =
           "normal_view", .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                        PropertySpec{.name = "ShadowMapSize",
                                     .typeInfo =
           Variant::getTypeInfo<glm::vec2>()},
                    },
                .outputSpecs =
                    {
                        PropertySpec{.name = "shadow_alias",
                                     .typeInfo =
           Variant::getTypeInfo<glm::vec3>()},
                    },
                .snippet = R"(
                    void shadowAlias(
                        in vec4 position_shadow_view, in vec3 normal_view,
                        in vec2 ShadowMapSize,
                        out vec3 alias)
                    {
                        vec2 proper_coordinate = (position_shadow_view.xy * 0.5
           + 0.5) * ShadowMapSize; vec2 diff = (gl_FragCoord.xy -
           proper_coordinate); vec2 normal_2d = normalize(normal_view.xy); float
           normal_based_offset = dot(normal_2d, diff) + 0.5; alias =
           vec3(normal_2d.xy * sign(normal_based_offset),
           normal_based_offset*normal_based_offset);
                    }
                )",
                .functionName = "shadowAlias"}});*/

        auto p_viewNormal2D =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            PropertySpec{.name = "position_shadow_view",
                                         .typeInfo =
                                             StandardShaderPropertyTypes::
                                                 FRAGMENT_OUTPUT},
                            PropertySpec{.name = "normal_view",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                            PropertySpec{.name = "ShadowMapSize",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec2>()},
                        },
                    .outputSpecs =
                        {
                            PropertySpec{.name = "shadow_alias",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                        },
                    .snippet = R"(
                        normal_view2d = normalize(normal_view.xy);
                )"}});

        auto p_shadowLightFactor =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            PropertySpec{.name = "tex_shadow",
                                         .typeInfo = Variant::getTypeInfo<
                                             dynasma::FirmPtr<Texture>>()},
                            PropertySpec{.name = "tex_shadow_normal",
                                         .typeInfo = Variant::getTypeInfo<
                                             dynasma::FirmPtr<Texture>>()},
                            PropertySpec{.name = "position_shadow",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                            PropertySpec{.name = "ebr_strength",
                                         .typeInfo =
                                             Variant::getTypeInfo<float>()},
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

                        vec2 normal2d = normalize(texture(tex_shadow_normal, position_shadow.xy).xy) / shadowSize;
                        vec2 tangent2d = vec2(normal2d.y, -normal2d.x);

                        const int numSamples = 4;
                        const float sampleRange = 2.0;

                        for (int i = 0; i < 4; i++) {
                            float t = -sampleRange + float(i) / numSamples * 2.0 * sampleRange;

                            vec2 pos_sample = position_shadow.xy + t * tangent2d;

                            vec2 texelPos = pos_sample * shadowSize;
                            vec2 bilinOffset = texelPos - floor(texelPos);

                            bvec4 inShadow = lessThan(
                                textureGather(tex_shadow, pos_sample, 0),
                                position_shadow.z + offset
                            );

                            light_shadow_factor += (
                                inShadow.w * (1.0 - bilinOffset.x) + inShadow.z * bilinOffset.x
                            ) * (1.0 - bilinOffset.y) + (
                                inShadow.x * (1.0 - bilinOffset.x) + inShadow.y * bilinOffset.x
                            ) * bilinOffset.y;
                        }

                        light_shadow_factor /= numSamples;

                        
                        if (light_shadow_factor >= 0.5) {
                            light_shadow_factor = 1.0;
                        }
                        else {
                            light_shadow_factor = 0.0;
                        }
                )"}});

        p_genericShaderMethod =
            dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
                .tasks = {p_viewNormal2D, p_shadowLightFactor}, .friendlyName = "ShadowEBR"});

        /*
        COMPOSING
        */

        auto p_shadowClear = root.getComponent<ComposeClearRenderKeeper>().new_asset(
            {ComposeClearRender::SetupParams{.root = root,
                                             .backgroundColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
                                             .displayOutputPropertyName = "shadow_cleared"}});

        auto p_shadowRender = root.getComponent<ComposeSceneRenderKeeper>().new_asset(
            {ComposeSceneRender::SetupParams{
                .root = root,
                .sceneInputPropertyName = "scene",
                .vertexPositionOutputPropertyName = "position_shadow_view",
                .displayInputPropertyName = "shadow_cleared",
                .displayOutputPropertyName = "rendered_shadow",
                .cullingMode = CullingMode::Frontface,
                .rasterizingMode = RasterizingMode::DerivationalFillCenters,
            }});
        auto p_shadowTexture =
            dynasma::makeStandalone<ComposeFrameToTexture>(ComposeFrameToTexture::SetupParams{
                .root = root,
                .frameInputPropertyName = "rendered_shadow",
                .depthTextureOutputPropertyName = "tex_shadow",
                .outputTexturePropertySpecs =
                    {
                        ComposeFrameToTexture::OutputTexturePropertySpec{
                            "tex_shadow_normal",
                            PropertySpec{"normal_view2d", Variant::getTypeInfo<glm::vec2>()}},
                    },
                .size = String("ShadowMapSize"),
                .channelType = Texture::ChannelType::VEC2_SNORM8,
                .horWrap = Texture::WrappingType::BORDER_COLOR,
                .verWrap = Texture::WrappingType::BORDER_COLOR,
                .minFilter = Texture::FilterType::LINEAR,
                .magFilter = Texture::FilterType::LINEAR,
                .useMipMaps = false});

        // compose method
        p_composeMethod =
            dynasma::makeStandalone<Method<ComposeTask>>(Method<ComposeTask>::MethodParams{
                .tasks = {p_shadowClear, p_shadowRender, p_shadowTexture},
                .friendlyName = "ShadowEBR"});
    }
};