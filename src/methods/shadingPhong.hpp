#pragma once

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/Assets/Mesh.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Shading/Snippet.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"

#include "dynasma/standalone.hpp"

namespace VitraeCommon
{
    using namespace Vitrae;

    inline void setupShadingPhong(ComponentRoot &root)
    {
        MethodCollection &methodCollection = root.getComponent<MethodCollection>();

        /*
        VERTEX SHADING
        */

        auto p_worldPosition =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {PropertySpec{
                             .name = "mat_model",
                             .typeInfo =
                                 StandardShaderPropertyTypes::INPUT_MODEL},
                         PropertySpec{
                             .name = "position",
                             .typeInfo = Variant::getTypeInfo<glm::vec3>()}},
                    .outputSpecs = {PropertySpec{
                        .name = "position_world",
                        .typeInfo = Variant::getTypeInfo<glm::vec4>()}},
                    .snippet = R"(
                        position_world = mat_model * vec4(position, 1.0);
                    )"}});
        methodCollection.registerShaderTask(p_worldPosition, ShaderStageFlag::Vertex);

        auto p_viewPosition =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            PropertySpec{
                                .name = "mat_camera_view",
                                .typeInfo =
                                    StandardShaderPropertyTypes::INPUT_VIEW},
                            PropertySpec{.name = "mat_camera_proj",
                                         .typeInfo =
                                             StandardShaderPropertyTypes::
                                                 INPUT_PROJECTION},
                            PropertySpec{.name = "position_world",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec4>()},
                        },
                    .outputSpecs = {PropertySpec{
                        .name = "position_view",
                        .typeInfo =
                            StandardShaderPropertyTypes::VERTEX_OUTPUT}},
                    .snippet = R"(
                        position_view = mat_camera_proj * mat_camera_view * position_world;
                    )"}});
        methodCollection.registerShaderTask(p_viewPosition, ShaderStageFlag::Vertex);

        auto p_viewNormal = root.getComponent<ShaderSnippetKeeper>().new_asset(
            {ShaderSnippet::StringParams{
                .inputSpecs =
                    {
                        PropertySpec{
                            .name = "mat_model",
                            .typeInfo =
                                StandardShaderPropertyTypes::INPUT_MODEL},
                        PropertySpec{
                            .name = "mat_camera_view",
                            .typeInfo =
                                StandardShaderPropertyTypes::INPUT_VIEW},
                        PropertySpec{
                            .name = "mat_camera_proj",
                            .typeInfo =
                                StandardShaderPropertyTypes::INPUT_PROJECTION},
                        PropertySpec{.name = "normal",
                                     .typeInfo =
                                         Variant::getTypeInfo<glm::vec3>()},
                    },
                .outputSpecs = {PropertySpec{
                    .name = "normal_view",
                    .typeInfo = Variant::getTypeInfo<glm::vec3>()}},
                .snippet = R"(
                    mat4 mat_viewproj = mat_camera_proj * mat_camera_view * mat_model;
                    vec4 origin_h = mat_viewproj * vec4(0.0, 0.0, 0.0, 1.0);
                    vec4 normal_h = mat_viewproj * vec4(normal, 1.0);
                    normal_view = normalize(mat3(mat_viewproj) * normal);
                )"}});
        methodCollection.registerShaderTask(p_viewNormal, ShaderStageFlag::Vertex);

        /*
        FRAGMENT/GENERIC SHADING
        */

        auto p_shadeDiffuse =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            PropertySpec{.name = "normal",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                            PropertySpec{.name = "light_direction",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                            PropertySpec{.name = "light_color_primary",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                            PropertySpec{.name = "light_shadow_factor",
                                         .typeInfo =
                                             Variant::getTypeInfo<float>()},
                        },
                    .outputSpecs =
                        {
                            PropertySpec{.name = "shade_diffuse",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                        },
                    .snippet = R"(
                        shade_diffuse = max(0.0, -dot(light_direction, normal)) * light_color_primary * light_shadow_factor;
                    )"}});
        methodCollection.registerShaderTask(p_shadeDiffuse, ShaderStageFlag::Fragment | ShaderStageFlag::Compute);

        auto p_shadeSpecular =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            PropertySpec{.name = "camera_position",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                            PropertySpec{.name = "position_world",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec4>()},
                            PropertySpec{
                                .name = StandardMaterialTextureNames::SPECULAR,
                                .typeInfo = Variant::getTypeInfo<
                                    dynasma::FirmPtr<Texture>>()},
                            PropertySpec{
                                .name =
                                    StandardVertexBufferNames::TEXTURE_COORD,
                                .typeInfo = Variant::getTypeInfo<glm::vec2>()},
                            PropertySpec{
                                .name =
                                    StandardMaterialPropertyNames::COL_SPECULAR,
                                .typeInfo = StandardMaterialPropertyTypes::
                                    COL_SPECULAR},
                            PropertySpec{
                                .name =
                                    StandardMaterialPropertyNames::SHININESS,
                                .typeInfo =
                                    StandardMaterialPropertyTypes::SHININESS},
                            PropertySpec{.name = "normal",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                            PropertySpec{.name = "light_direction",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                            PropertySpec{.name = "light_color_primary",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                            PropertySpec{.name = "light_shadow_factor",
                                         .typeInfo =
                                             Variant::getTypeInfo<float>()},
                        },
                    .outputSpecs =
                        {
                            PropertySpec{.name = "shade_specular",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                        },
                    .snippet = R"(
                        vec4 color_specular_tot = texture(tex_specular, textureCoord0);
                        vec3 dirToEye = normalize(camera_position - position_world.xyz);
                        vec3 reflRay = 2 * dot(-light_direction, normal) * normal + light_direction;
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
                            PropertySpec{.name = "shade_diffuse",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                            PropertySpec{.name = "shade_specular",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                            PropertySpec{.name = "shade_ambient",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                            PropertySpec{
                                .name = StandardMaterialTextureNames::DIFFUSE,
                                .typeInfo = Variant::getTypeInfo<
                                    dynasma::FirmPtr<Texture>>()},
                            PropertySpec{
                                .name =
                                    StandardVertexBufferNames::TEXTURE_COORD,
                                .typeInfo = Variant::getTypeInfo<glm::vec2>()},
                        },
                    .outputSpecs =
                        {
                            PropertySpec{
                                .name = "phong_shade",
                                .typeInfo = StandardShaderPropertyTypes::
                                    FRAGMENT_OUTPUT},
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