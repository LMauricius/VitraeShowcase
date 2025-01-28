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

    inline void setupShadingPhong(ComponentRoot &root)
    {
        MethodCollection &methodCollection = root.getComponent<MethodCollection>();

        auto p_shadeDiffuse =
            root.getComponent<ShaderSnippetKeeper>().new_asset({ShaderSnippet::StringParams{
                .inputSpecs =
                    {
                        {"normal_fragment_normalized", TYPE_INFO<glm::vec3>},
                        {"light_direction", TYPE_INFO<glm::vec3>},
                        {"light_color_primary", TYPE_INFO<glm::vec3>},
                        {"light_shadow_factor", TYPE_INFO<float>},
                    },
                .outputSpecs =
                    {
                        {"shade_diffuse", TYPE_INFO<glm::vec3>},
                    },
                .snippet = R"glsl(
                    shade_diffuse = max(0.0, -dot(light_direction, normal_fragment_normalized)) * light_color_primary * light_shadow_factor;
                )glsl",
            }});
        methodCollection.registerShaderTask(p_shadeDiffuse, ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

        methodCollection.registerShaderTask(
            root.getComponent<ShaderSnippetKeeper>().new_asset({ShaderSnippet::StringParams{
                .inputSpecs =
                    {
                        {"camera_position", TYPE_INFO<glm::vec3>},
                        {"position_world", TYPE_INFO<glm::vec4>},
                        StandardParam::color_specular,
                        StandardParam::color_shininess,
                        {"normal_fragment_normalized", TYPE_INFO<glm::vec3>},
                        {"light_direction", TYPE_INFO<glm::vec3>},
                        {"light_color_primary", TYPE_INFO<glm::vec3>},
                        {"light_shadow_factor", TYPE_INFO<float>},
                    },
                .outputSpecs =
                    {
                        {"shade_specular", TYPE_INFO<glm::vec3>},
                    },
                .snippet = R"glsl(
                    vec3 dirToEye = normalize(camera_position - position_world.xyz);
                    vec3 reflRay = 2 * dot(-light_direction, normal_fragment_normalized) * normal_fragment_normalized + light_direction;
                    shade_specular =
                        pow(max(dot(reflRay, dirToEye), 0.001), color_shininess.x) * light_shadow_factor *
                        color_specular.rgb * light_color_primary;
                )glsl",
            }}),
            ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

        methodCollection.registerShaderTask(
            root.getComponent<ShaderSnippetKeeper>().new_asset({ShaderSnippet::StringParams{
                .inputSpecs =
                    {
                        {"shade_diffuse", TYPE_INFO<glm::vec3>},
                        {"shade_specular", TYPE_INFO<glm::vec3>},
                        {"shade_ambient", TYPE_INFO<glm::vec3>},
                        StandardParam::color_diffuse,
                    },
                .outputSpecs =
                    {
                        {"phong_shade", TYPE_INFO<glm::vec4>},
                    },
                .snippet = R"glsl(
                    phong_shade = vec4(
                        color_diffuse.rgb * (shade_diffuse + shade_ambient) +
                        shade_specular, 
                        color_diffuse.a);
                )glsl",
            }}),
            ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

        methodCollection.registerPropertyOption("shade_ambient", "light_color_ambient");
        methodCollection.registerPropertyOption("shade", "phong_shade");
    }
}