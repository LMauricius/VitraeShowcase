#pragma once

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Compositing/AdaptTasks.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"

#include "dynasma/standalone.hpp"

namespace VitraeCommon
{
    using namespace Vitrae;

    inline void setupAssimpPBSConvert(ComponentRoot &root)
    {
        root.addAiMaterialParamAliases(
            aiShadingMode_PBR_BRDF,
            {{"shade", "pbs_shade"}});

        /*
        Texture types
        */
        root.addAiMaterialTextureInfo(
            {StandardParam::tex_base.name, aiTextureType_BASE_COLOR,
             root.getComponent<TextureManager>().register_asset(
                 {Texture::PureColorParams{.root = root, .color = {1.0f, 1.0f, 1.0f, 1.0f}}})});
        root.addAiMaterialTextureInfo(
            {StandardParam::tex_metalness.name, aiTextureType_METALNESS,
             root.getComponent<TextureManager>().register_asset(
                 {Texture::PureColorParams{.root = root, .color = {0.0f, 0.0f, 1.0f, 1.0f}}})});
        root.addAiMaterialTextureInfo(
            {StandardParam::tex_smoothness.name, aiTextureType_DIFFUSE_ROUGHNESS,
             root.getComponent<TextureManager>().register_asset(
                 {Texture::PureColorParams{.root = root, .color = {0.0f, 0.0f, 0.0f, 1.0f}}})});
    }
}