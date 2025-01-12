#pragma once

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Compositing/AdaptTasks.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"

#include "dynasma/standalone.hpp"

namespace VitraeCommon
{
    using namespace Vitrae;

    inline void setupShadowBiLin(ComponentRoot &root)
    {
        MethodCollection &methodCollection = root.getComponent<MethodCollection>();

        auto p_shadowLightFactor =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            ParamSpec{.name = "tex_shadow",
                                      .typeInfo = TYPE_INFO<
                                          dynasma::FirmPtr<Texture>>},
                            ParamSpec{.name = "position_shadow",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec3>},
                        },
                    .outputSpecs =
                        {
                            ParamSpec{.name = "light_shadow_factor_bilin",
                                      .typeInfo =
                                          TYPE_INFO<float>},
                        },
                    .snippet = R"(
                        vec2 shadowSize = textureSize(tex_shadow, 0);
                        float offset = 0.5 / shadowSize.x;
                        vec2 texelPos = position_shadow.xy * shadowSize;
                        vec2 bilinOffset = texelPos - floor(texelPos);

                        bvec4 inShadow = lessThan(
                            textureGather(tex_shadow, position_shadow.xy - offset, 0),
                            vec4(position_shadow.z + offset)
                        );

                        light_shadow_factor_bilin = (
                            (inShadow.w? 0.0 : 1.0 - bilinOffset.x) + (inShadow.z? 0.0 : bilinOffset.x)
                        ) * (1.0 - bilinOffset.y) + (
                            (inShadow.x? 0.0 : 1.0 - bilinOffset.x) + (inShadow.y? 0.0 : bilinOffset.x)
                        ) * bilinOffset.y;
                )"}});
        methodCollection.registerShaderTask(p_shadowLightFactor, ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

        methodCollection.registerPropertyOption("light_shadow_factor", "light_shadow_factor_bilin");
    }
}