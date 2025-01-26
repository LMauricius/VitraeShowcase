#pragma once

#include "../Standard/Params.hpp"

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"
#include "Vitrae/Pipelines/Compositing/AdaptTasks.hpp"
#include "Vitrae/Pipelines/Shading/Snippet.hpp"

#include "dynasma/standalone.hpp"

namespace VitraeCommon
{
    using namespace Vitrae;

    inline void setupDisplayNormals(ComponentRoot &root)
    {
        MethodCollection &methodCollection = root.getComponent<MethodCollection>();

        auto p_normal =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            {.name = "normal_fragment_normalized",
                             .typeInfo = TYPE_INFO<glm::vec3>},
                        },
                    .outputSpecs =
                        {
                            ParamSpec{.name = "normal_visualized",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec4>},
                        },
                    .snippet = R"(
                        normal_visualized = vec4(normal_fragment_normalized * 0.5 + 0.5, 1.0);
                    )"}});
        methodCollection.registerShaderTask(p_normal, ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

        auto p_renderAdaptor = dynasma::makeStandalone<ComposeAdaptTasks>(
            ComposeAdaptTasks::SetupParams{.root = root,
                                           .adaptorAliases = {
                                               {"normals_displayed", "scene_rendered"},
                                               {"position_view", "position_camera_view"},
                                               {"fs_target", "fs_display"},
                                               {"phong_shade", "normal_visualized"},
                                           },
                                           .desiredOutputs = {ParamSpec{
                                               "normals_displayed",
                                               TYPE_INFO<void>,
                                           }},
                                           .friendlyName = "Render normals"});
        methodCollection.registerComposeTask(p_renderAdaptor);

        methodCollection.registerCompositorOutput("normals_displayed");
    }
}