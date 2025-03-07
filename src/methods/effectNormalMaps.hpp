#pragma once

#include "../Standard/Params.hpp"

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/Assets/Shapes/Mesh.hpp"
#include "Vitrae/Params/Standard.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Shading/Snippet.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"

#include "dynasma/standalone.hpp"

namespace VitraeCommon
{
    using namespace Vitrae;

    inline void setupEffectNormalMaps(ComponentRoot &root)
    {
        MethodCollection &methodCollection = root.getComponent<MethodCollection>();

        /*
        VERTEX SHADING
        */

        methodCollection.registerShaderTask(
            root.getComponent<ShaderSnippetKeeper>().new_asset({ShaderSnippet::StringParams{
                .inputSpecs =
                    {
                        StandardParam::mat_model,
                        StandardParam::tangent,
                        StandardParam::bitangent,
                    },
                .outputSpecs =
                    {
                        {"tangent_world", TYPE_INFO<glm::vec3>},
                        {"bitangent_world", TYPE_INFO<glm::vec3>},
                    },
                .snippet = R"glsl(
                    tangent_world = mat3(mat_model) * tangent;
                    bitangent_world = mat3(mat_model) * bitangent;
                )glsl",
            }}),
            ShaderStageFlag::Vertex);

        /*
        FRAGMENT/GENERIC SHADING
        */

        methodCollection.registerShaderTask(
            root.getComponent<ShaderSnippetKeeper>().new_asset({ShaderSnippet::StringParams{
                .inputSpecs =
                    {
                        {"normal_world", TYPE_INFO<glm::vec3>},
                        {"tangent_world", TYPE_INFO<glm::vec3>},
                        {"bitangent_world", TYPE_INFO<glm::vec3>},
                        {"color_normal", TYPE_INFO<glm::vec4>},
                    },
                .outputSpecs =
                    {
                        ParamSpec{.name = "normal_worldPixelMapped",
                                  .typeInfo = TYPE_INFO<glm::vec3>},
                    },
                .snippet = R"glsl(
                    vec2 adj_normal = color_normal.xy * 2.0 - 1.0;
                    normal_worldPixelMapped = -adj_normal.x * tangent_world +
                                              -adj_normal.y * bitangent_world +
                                              color_normal.z * normal_world;
                )glsl",
            }}),
            ShaderStageFlag::Fragment | ShaderStageFlag::Compute);
        methodCollection.registerPropertyOption("normal_fragment", "normal_worldPixelMapped");
    }
}