#pragma once

#include "../Standard/Params.hpp"

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

    inline void setupAssimpPhongConvert(ComponentRoot &root)
    {
        root.addAiMaterialParamAliases(
            aiShadingMode_Phong,
            {{"shade", "phong_shade"}});
    }
}