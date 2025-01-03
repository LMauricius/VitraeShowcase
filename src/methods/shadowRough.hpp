#pragma once

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Compositing/AdaptTasks.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"

#include "dynasma/standalone.hpp"

namespace VitraeCommon
{
    using namespace Vitrae;

    inline void setupShadowRough(ComponentRoot &root)
    {
        MethodCollection &methodCollection = root.getComponent<MethodCollection>();

        auto p_shadowLightFactor =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            PropertySpec{.name = "tex_shadow",
                                         .typeInfo = Variant::getTypeInfo<
                                             dynasma::FirmPtr<Texture>>()},
                            PropertySpec{.name = "position_shadow",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                        },
                    .outputSpecs =
                        {
                            PropertySpec{.name = "light_shadow_factor_rough",
                                         .typeInfo =
                                             Variant::getTypeInfo<float>()},
                        },
                    .snippet = R"(
                        vec2 shadowSize = textureSize(tex_shadow, 0);
                        ivec2 texelPosI = ivec2(round(position_shadow.xy * shadowSize));
                        float offset = 0.5 / shadowSize.x;
                        if (texelFetch(tex_shadow, texelPosI, 0).r < position_shadow.z + offset) {
                            light_shadow_factor_rough = 0.0;
                        }
                        else {
                            light_shadow_factor_rough = 1.0;
                        }
                )"}});
        methodCollection.registerShaderTask(p_shadowLightFactor, ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

        methodCollection.registerPropertyOption("light_shadow_factor", "light_shadow_factor_rough");
    }
}