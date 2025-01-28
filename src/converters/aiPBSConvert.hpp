#pragma once

#include "../Standard/Params.hpp"
#include "../Standard/Textures.hpp"

#include "Vitrae/Assets/Texture.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"
#include "Vitrae/Pipelines/Compositing/AdaptTasks.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"

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
        root.addAiMaterialParamAliases(aiShadingMode_Phong, {{"shade", "phong_shade"}});

        root.addAiMaterialTextureInfo({
            StandardTexture::base,
            aiTextureType_BASE_COLOR,
            {1.0f, 1.0f, 1.0f, 1.0f},
        });
        root.addAiMaterialTextureInfo({
            StandardTexture::metalness,
            aiTextureType_METALNESS,
            {0.0f, 0.0f, 0.0f, 1.0f},
        });
        root.addAiMaterialTextureInfo({
            StandardTexture::smoothness,
            aiTextureType_DIFFUSE_ROUGHNESS,
            {0.0f, 0.0f, 0.0f, 1.0f},
        });
    }
}