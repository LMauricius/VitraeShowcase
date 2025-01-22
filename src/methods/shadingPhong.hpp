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
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            ParamSpec{.name = "normal_fragment_normalized",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec3>},
                            ParamSpec{.name = "light_direction",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec3>},
                            ParamSpec{.name = "light_color_primary",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec3>},
                            ParamSpec{.name = "light_shadow_factor",
                                      .typeInfo =
                                          TYPE_INFO<float>},
                        },
                    .outputSpecs =
                        {
                            ParamSpec{.name = "shade_diffuse",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec3>},
                        },
                    .snippet = R"(
                        shade_diffuse = max(0.0, -dot(light_direction, normal_fragment_normalized)) * light_color_primary * light_shadow_factor;
                    )"}});
        methodCollection.registerShaderTask(p_shadeDiffuse, ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

        auto p_shadeSpecular =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            ParamSpec{.name = "camera_position",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec3>},
                            ParamSpec{.name = "position_world",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec4>},
                            ParamSpec{
                                .name = StandardMaterialTextureNames::SPECULAR,
                                .typeInfo = TYPE_INFO<
                                    dynasma::FirmPtr<Texture>>},
                            ParamSpec{
                                .name =
                                    StandardVertexBufferNames::TEXTURE_COORD,
                                .typeInfo = TYPE_INFO<glm::vec2>},
                            ParamSpec{
                                .name =
                                    StandardMaterialPropertyNames::COL_SPECULAR,
                                .typeInfo = StandardMaterialPropertyTypes::
                                    COL_SPECULAR},
                            ParamSpec{
                                .name =
                                    StandardMaterialPropertyNames::SHININESS,
                                .typeInfo =
                                    StandardMaterialPropertyTypes::SHININESS},
                            ParamSpec{.name = "normal_fragment_normalized",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec3>},
                            ParamSpec{.name = "light_direction",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec3>},
                            ParamSpec{.name = "light_color_primary",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec3>},
                            ParamSpec{.name = "light_shadow_factor",
                                      .typeInfo =
                                          TYPE_INFO<float>},
                        },
                    .outputSpecs =
                        {
                            ParamSpec{.name = "shade_specular",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec3>},
                        },
                    .snippet = R"(
                        vec4 color_specular_tot = texture(tex_specular, textureCoord0);
                        vec3 dirToEye = normalize(camera_position - position_world.xyz);
                        vec3 reflRay = 2 * dot(-light_direction, normal_fragment_normalized) * normal_fragment_normalized + light_direction;
                        shade_specular =
                            pow(max(dot(reflRay, dirToEye), 0.001), shininess) * light_shadow_factor *
                            color_specular_tot.rgb * light_color_primary;
                )"}});
        methodCollection.registerShaderTask(p_shadeSpecular, ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

        auto p_phongCombine =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            ParamSpec{.name = "shade_diffuse",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec3>},
                            ParamSpec{.name = "shade_specular",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec3>},
                            ParamSpec{.name = "shade_ambient",
                                      .typeInfo =
                                          TYPE_INFO<glm::vec3>},
                            ParamSpec{
                                .name = StandardMaterialTextureNames::DIFFUSE,
                                .typeInfo = TYPE_INFO<
                                    dynasma::FirmPtr<Texture>>},
                            ParamSpec{
                                .name =
                                    StandardVertexBufferNames::TEXTURE_COORD,
                                .typeInfo = TYPE_INFO<glm::vec2>},
                        },
                    .outputSpecs =
                        {
                            ParamSpec{
                                .name = "phong_shade",
                                .typeInfo = TYPE_INFO<glm::vec4>},
                        },
                    .snippet = R"(
                        vec4 color_diffuse = texture(tex_diffuse, textureCoord0);
                        phong_shade = vec4(
                            color_diffuse.rgb * (shade_diffuse + shade_ambient) +
                            shade_specular, 
                            color_diffuse.a);
                )"}});
        methodCollection.registerShaderTask(p_phongCombine, ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

        methodCollection.registerPropertyOption("shade_ambient", "light_color_ambient");
        methodCollection.registerPropertyOption("shade", "phong_shade");
    }
}