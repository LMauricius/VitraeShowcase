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

    inline void setupShadowPCF(ComponentRoot &root)
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
                            ParamSpec{.name = "light_shadow_factor_PCF",
                                      .typeInfo =
                                          TYPE_INFO<float>},
                        },
                    .snippet = R"(
                        const int maskSize = 2;
                        const float blurRadius = 0.8;

                        vec2 shadowSize = textureSize(tex_shadow, 0);

                        vec2 stride = blurRadius / float(maskSize) / shadowSize;

                        int counter = 0;
                        light_shadow_factor_PCF = 0;
                        for (int i = -maskSize; i <= maskSize; ++i) {
                            int shift = int((i % 2) == 0);
                            for (int j = -maskSize+shift; j <= maskSize-shift; j += 2) {
                                if (texture(tex_shadow, position_shadow.xy + vec2(ivec2(i, j)) * stride).r < position_shadow.z + 0.5/shadowSize.x) {
                                    light_shadow_factor_PCF += 0.0;
                                }
                                else {
                                    light_shadow_factor_PCF += 1.0;
                                }
                                ++counter;
                            }
                        }
                        light_shadow_factor_PCF /= float(counter);
                )"}});
        methodCollection.registerShaderTask(p_shadowLightFactor, ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

        methodCollection.registerPropertyOption("light_shadow_factor", "light_shadow_factor_PCF");
    }
}