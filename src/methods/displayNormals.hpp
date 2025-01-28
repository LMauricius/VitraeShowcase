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

        methodCollection.registerShaderTask(
            root.getComponent<ShaderSnippetKeeper>().new_asset({ShaderSnippet::StringParams{
                .inputSpecs =
                    {
                        {"normal_fragment_normalized", TYPE_INFO<glm::vec3>},
                    },
                .outputSpecs =
                    {
                        {"normal_visualized", TYPE_INFO<glm::vec4>},
                    },
                .snippet = R"glsl(
                    normal_visualized = vec4(normal_fragment_normalized * 0.5 + 0.5, 1.0);
                )glsl",
            }}),
            ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

        methodCollection.registerPropertyOption(StandardParam::fragment_color.name,
                                                "normal_visualized");
    }
}