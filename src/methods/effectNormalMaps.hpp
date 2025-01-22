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

        auto p_worldTangents =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            StandardParam::mat_model,
                            StandardParam::tangent,
                            StandardParam::bitangent,
                        },
                    .outputSpecs = {
                        {.name = "tangent_world",
                         .typeInfo = TYPE_INFO<glm::vec3>},
                        {.name = "bitangent_world",
                         .typeInfo = TYPE_INFO<glm::vec3>},
                    },
                    .snippet = R"(
                        tangent_world = mat3(mat_model) * tangent;
                        bitangent_world = mat3(mat_model) * bitangent;
                    )"}});
        methodCollection.registerShaderTask(p_worldTangents, ShaderStageFlag::Vertex);

        /*
        FRAGMENT/GENERIC SHADING
        */

        auto p_normal =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            {.name = "normal_world",
                             .typeInfo = TYPE_INFO<glm::vec3>},
                            {.name = "tangent_world",
                             .typeInfo = TYPE_INFO<glm::vec3>},
                            {.name = "bitangent_world",
                             .typeInfo = TYPE_INFO<glm::vec3>},
                            {.name = "tex_normal",
                             .typeInfo = TYPE_INFO<dynasma::FirmPtr<Texture>>},
                            {.name = "textureCoord0",
                             .typeInfo = TYPE_INFO<glm::vec2>},
                        },
                    .outputSpecs =
                        {
                            ParamSpec{.name = "normal_worldPixelMapped",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec3>},
                        },
                    .snippet = R"(
                        vec3 sample_normal = texture(tex_normal, textureCoord0).xyz * 2.0 - 1.0;
                        normal_worldPixelMapped = sample_normal.x * tangent_world +
                                                  sample_normal.y * bitangent_world +
                                                  sample_normal.z * normal_world;
                    )"}});
        methodCollection.registerShaderTask(p_normal, ShaderStageFlag::Fragment | ShaderStageFlag::Compute);
        methodCollection.registerPropertyOption("normal_fragment", "normal_worldPixelMapped");
    }
}