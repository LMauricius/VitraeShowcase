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

struct MethodsRenderShades : MethodCollection
{
    inline MethodsRenderShades(ComponentRoot &root, bool wireframe, bool edgeSmoothing, String name)
        : MethodCollection(root)
    {
        /*
        VERTEX SHADING
        */

        p_vertexMethod = dynasma::makeStandalone<Method<ShaderTask>>(
            Method<ShaderTask>::MethodParams{.tasks = {}});

        /*
        FRAGMENT SHADING
        */

        p_fragmentMethod = dynasma::makeStandalone<Method<ShaderTask>>(
            Method<ShaderTask>::MethodParams{.tasks = {}});

        /*
        COMPOSING
        */

        // normal render

        auto p_clear = root.getComponent<ComposeClearRenderKeeper>().new_asset(
            {ComposeClearRender::SetupParams{.root = root,
                                             .backgroundColor = glm::vec4(0.0f, 0.5f, 0.8f, 1.0f),
                                             .displayOutputPropertyName = "display_cleared"}});

        auto p_normalRender = root.getComponent<ComposeSceneRenderKeeper>().new_asset(
            {ComposeSceneRender::SetupParams{
                .root = root,
                .sceneInputPropertyName = "scene",
                .vertexPositionOutputPropertyName = "position_view",
                .displayInputPropertyName = "display_cleared",
                .displayOutputPropertyName = StandardCompositorOutputNames::OUTPUT,
                .rasterizingMode = wireframe       ? RasterizingMode::DerivationalTraceEdges
                                   : edgeSmoothing ? RasterizingMode::DerivationalFillEdges
                                                   : RasterizingMode::DerivationalFillCenters,
                .smoothTracing = edgeSmoothing,
            }});

        p_composeMethod =
            dynasma::makeStandalone<Method<ComposeTask>>(Method<ComposeTask>::MethodParams{
                .tasks = {p_clear, p_normalRender}, .friendlyName = name});

        desiredOutputs =
            ParamList{ParamSpec{.name = StandardCompositorOutputNames::OUTPUT,
                                .typeInfo = StandardCompositorOutputTypes::OUTPUT_TYPE}};
    }
};