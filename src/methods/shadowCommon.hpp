#pragma once

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Compositing/AdaptTasks.hpp"
#include "Vitrae/Pipelines/Compositing/FrameToTexture.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"

#include "dynasma/standalone.hpp"

namespace VitraeCommon
{
    using namespace Vitrae;

    inline void setupShadowCommon(ComponentRoot &root)
    {
        MethodCollection &methodCollection = root.getComponent<MethodCollection>();

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
        methodCollection.registerShaderTask(p_shadowPosition, ShaderStageFlag::Vertex | ShaderStageFlag::Compute);

        auto p_shadowTexture = dynasma::makeStandalone<ComposeFrameToTexture>(
            ComposeFrameToTexture::SetupParams{.root = root,
                                               .inputTokenNames = {"scene_silhouette_rendered"},
                                               .outputColorTextureName = "",
                                               .outputDepthTextureName = "tex_shadow_adapted",
                                               .size = String("ShadowMapSize"),
                                               .horWrap = Texture::WrappingType::BORDER_COLOR,
                                               .verWrap = Texture::WrappingType::BORDER_COLOR,
                                               .minFilter = Texture::FilterType::NEAREST,
                                               .magFilter = Texture::FilterType::NEAREST,
                                               .useMipMaps = false});
        methodCollection.registerComposeTask(p_shadowTexture);

        auto p_shadowRenderAdaptor = dynasma::makeStandalone<ComposeAdaptTasks>(
            ComposeAdaptTasks::SetupParams{.root = root,
                                           .adaptorAliases = {
                                               {"fs_target", "fs_shadow"},
                                               {"position_view", "position_shadow_view"},
                                               {"tex_shadow", "tex_shadow_adapted"},
                                           },
                                           .desiredOutputs = {
                                               ParamSpec{
                                                   "tex_shadow",
                                                   TYPE_INFO<dynasma::FirmPtr<Texture>>,
                                               },
                                           },
                                           .friendlyName = "Render shadows"});
        methodCollection.registerComposeTask(p_shadowRenderAdaptor);
    }
}