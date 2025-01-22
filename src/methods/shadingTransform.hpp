#pragma once

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

    inline void setupSshadingTransform(ComponentRoot &root)
    {
        MethodCollection &methodCollection = root.getComponent<MethodCollection>();

        /*
        VERTEX SHADING
        */

        auto p_worldPosition =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            ::StandardParam::mat_model,
                            ::StandardParam::position,
                            ::StandardParam::normal,
                        },
                    .outputSpecs = {
                        {.name = "position_world",
                         .typeInfo = TYPE_INFO<glm::vec4>},
                        {.name = "normal_world",
                         .typeInfo = TYPE_INFO<glm::vec3>},
                    },
                    .snippet = R"(
                        position_world = mat_model * vec4(position, 1.0);
                        normal_world = normalize(mat3(mat_model) * normal);
                    )"}});
        methodCollection.registerShaderTask(p_worldPosition, ShaderStageFlag::Vertex);
        methodCollection.registerPropertyOption("normal_fragment", "normal_world");

        auto p_viewPosition =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            ::StandardParam::mat_display,
                            ParamSpec{.name = "position_world",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec4>},
                        },
                    .outputSpecs = {ParamSpec{
                        .name = "position_camera_view",
                        .typeInfo =
                            TYPE_INFO<glm::vec4>}},
                    .snippet = R"(
                        position_camera_view = mat_display * position_world;
                    )"}});
        methodCollection.registerShaderTask(p_viewPosition, ShaderStageFlag::Vertex);

        auto p_viewNormal = root.getComponent<ShaderSnippetKeeper>().new_asset(
            {ShaderSnippet::StringParams{
                .inputSpecs =
                    {
                        ::StandardParam::mat_mvp,
                        ParamSpec{.name = "normal",
                                  .typeInfo =
                                      TYPE_INFO<glm::vec3>},
                    },
                .outputSpecs = {ParamSpec{
                    .name = "normal_view",
                    .typeInfo = TYPE_INFO<glm::vec3>}},
                .snippet = R"(
                    vec4 origin_h = mat_mvp * vec4(0.0, 0.0, 0.0, 1.0);
                    vec4 normal_h = mat_mvp * vec4(normal, 1.0);
                    normal_view = normalize(mat3(mat_mvp) * normal);
                )"}});
        methodCollection.registerShaderTask(p_viewNormal, ShaderStageFlag::Vertex);

        /*
        FRAGMENT/GENERIC SHADING
        */
        auto p_normalizedNormal =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            {.name = "normal_fragment",
                             .typeInfo =
                                 TYPE_INFO<glm::vec3>},
                        },
                    .outputSpecs =
                        {
                            {.name = "normal_fragment_normalized",
                             .typeInfo =
                                 TYPE_INFO<glm::vec3>},
                        },
                    .snippet = R"(
                        normal_fragment_normalized = normalize(normal_fragment);
                    )"}});
        methodCollection.registerShaderTask(p_normalizedNormal, ShaderStageFlag::Fragment | ShaderStageFlag::Compute);
    }
}