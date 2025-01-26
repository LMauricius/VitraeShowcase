#pragma once

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/Assets/Shapes/Mesh.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"
#include "Vitrae/Params/Standard.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Shading/Snippet.hpp"

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

    methodCollection.registerShaderTask(
        root.getComponent<ShaderSnippetKeeper>().new_asset({ShaderSnippet::StringParams{
            .inputSpecs =
                {
                    ::StandardParam::mat_model,
                    ::StandardParam::position,
                    ::StandardParam::normal,
                },
            .outputSpecs =
                {
                    {"position_world", TYPE_INFO<glm::vec4>},
                    {"normal_world", TYPE_INFO<glm::vec3>},
                },
            .snippet = R"glsl(
                position_world = mat_model * vec4(position, 1.0);
                normal_world = normalize(mat3(mat_model) * normal);
            )glsl",
        }}),
        ShaderStageFlag::Vertex);
    methodCollection.registerPropertyOption("normal_fragment", "normal_world");

    methodCollection.registerShaderTask(
        root.getComponent<ShaderSnippetKeeper>().new_asset({ShaderSnippet::StringParams{
            .inputSpecs =
                {
                    ::StandardParam::mat_display,
                    {"position_world", TYPE_INFO<glm::vec4>},
                },
            .outputSpecs =
                {
                    {"position_camera_view", TYPE_INFO<glm::vec4>},
                },
            .snippet = R"glsl(
                position_camera_view = mat_display * position_world;
            )glsl",
        }}),
        ShaderStageFlag::Vertex);

    methodCollection.registerShaderTask(
        root.getComponent<ShaderSnippetKeeper>().new_asset({ShaderSnippet::StringParams{
            .inputSpecs =
                {
                    ::StandardParam::mat_mvp,
                    ParamSpec{.name = "normal", .typeInfo = TYPE_INFO<glm::vec3>},
                },
            .outputSpecs = {ParamSpec{.name = "normal_view", .typeInfo = TYPE_INFO<glm::vec3>}},
            .snippet = R"glsl(
                vec4 origin_h = mat_mvp * vec4(0.0, 0.0, 0.0, 1.0);
                vec4 normal_h = mat_mvp * vec4(normal, 1.0);
                normal_view = normalize(mat3(mat_mvp) * normal);
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
                    {.name = "normal_fragment", .typeInfo = TYPE_INFO<glm::vec3>},
                },
            .outputSpecs =
                {
                    {.name = "normal_fragment_normalized", .typeInfo = TYPE_INFO<glm::vec3>},
                },
            .snippet = R"glsl(
                normal_fragment_normalized = normalize(normal_fragment);
            )glsl",
        }}),
        ShaderStageFlag::Fragment | ShaderStageFlag::Compute);
}
} // namespace VitraeCommon