#pragma once

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Compositing/AdaptTasks.hpp"
#include "Vitrae/Pipelines/Compositing/FrameToTexture.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"

#include "dynasma/standalone.hpp"

namespace VitraeCommon
{
    using namespace Vitrae;

    inline void setupShadowCommon(ComponentRoot &root)
    {
        MethodCollection &methodCollection = root.getComponent<MethodCollection>();

        auto p_shadowTexture = dynasma::makeStandalone<ComposeFrameToTexture>(
            ComposeFrameToTexture::SetupParams{.root = root,
                                               .outputColorTextureName = "",
                                               .outputDepthTextureName = "tex_shadow",
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
                                               {"shadowmaps_rendered", "scene_silouette_rendered"},
                                               {"position_view", "position_shadow_view"},
                                           },
                                           .desiredOutputs = {
                                               PropertySpec{
                                                   "shadowmaps_rendered",
                                                   Variant::getTypeInfo<void>(),
                                               },
                                               PropertySpec{
                                                   "fs_shadow",
                                                   Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>(),
                                               },
                                           },
                                           .friendlyName = "Render shadows"});
        methodCollection.registerComposeTask(p_shadowRenderAdaptor);
    }
}