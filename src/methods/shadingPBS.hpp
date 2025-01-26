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

    inline void setupShadingPBS(ComponentRoot &root)
    {
        MethodCollection &methodCollection = root.getComponent<MethodCollection>();

        methodCollection.registerShaderTask(
            root.getComponent<ShaderSnippetKeeper>().new_asset({
                ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            {"camera_position", TYPE_INFO<glm::vec3>},
                            {"position_world", TYPE_INFO<glm::vec4>},
                            StandardParam::tex_base,
                            StandardParam::textureCoord0,
                            StandardParam::tex_metalness,
                            StandardParam::tex_smoothness,
                            {"shade_ambient", TYPE_INFO<glm::vec3>},
                            {"normal_fragment_normalized", TYPE_INFO<glm::vec3>},
                            {"light_direction", TYPE_INFO<glm::vec3>},
                            {"light_color_primary", TYPE_INFO<glm::vec3>},
                            {"light_shadow_factor", TYPE_INFO<float>},
                            {"light_shadow_factor", TYPE_INFO<float>},
                        },
                    .outputSpecs =
                        {
                            {"pbs_shade", TYPE_INFO<glm::vec4>},
                        },
                    .snippet = R"glsl(
                        vec3 sample_base = texture(tex_base, textureCoord0).rgb;
                        float sample_metalness = texture(tex_metalness, textureCoord0).g;
                        float sample_smoothness = texture(tex_smoothness, textureCoord0).g;
                        
                        vec3 dirToEye = normalize(camera_position - position_world.xyz);
                        vec3 reflRay = 2 * dot(-light_direction, normal_fragment_normalized) * normal_fragment_normalized + light_direction;

                        float alpha = 1.0 - sample_smoothness;
                        float Kd = 1.0 - sample_smoothness;
                        float Ks = sample_smoothness;
                        vec3 f_lambert = sample_base / 3.1415;
                        vec3 f_cooktorrance = dfg / (4.0 * dot(dirToEye, normal_fragment_normalized) * dot(light_direction, normal_fragment_normalized));

                        pbs_shade =
                            pow(max(dot(reflRay, dirToEye), 0.001), -log(1.0-sample_smoothness)) * light_shadow_factor *
                            sample_base.rgb * light_color_primary;
                    )glsl",
                },
            }),
            ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

        methodCollection.registerPropertyOption("shade", "pbs_shade");
    }
}