#pragma once

#include "../shadingModes.hpp"
#include "abstract.hpp"

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/Assets/Mesh.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/FrameToTexture.hpp"
#include "Vitrae/Pipelines/Compositing/Function.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Shading/Constant.hpp"
#include "Vitrae/Pipelines/Shading/Snippet.hpp"

#include "dynasma/standalone.hpp"

#include <iostream>

using namespace Vitrae;

struct MethodsClassic : MethodCollection
{
    inline MethodsClassic(ComponentRoot &root) : MethodCollection(root)
    {
        /*
        VERTEX SHADING
        */

        auto p_worldPosition =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {PropertySpec{
                             .name = StandardShaderPropertyNames::INPUT_MODEL,
                             .typeInfo =
                                 StandardShaderPropertyTypes::INPUT_MODEL},
                         PropertySpec{
                             .name = StandardVertexBufferNames::POSITION,
                             .typeInfo = Variant::getTypeInfo<glm::vec3>()}},
                    .outputSpecs = {PropertySpec{
                        .name = "position_world",
                        .typeInfo = Variant::getTypeInfo<glm::vec4>()}},
                    .snippet = R"(
                        position_world = mat_model * vec4(position, 1.0);
                    )"}});

        auto p_viewPosition =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            PropertySpec{
                                .name = StandardShaderPropertyNames::INPUT_VIEW,
                                .typeInfo =
                                    StandardShaderPropertyTypes::INPUT_VIEW},
                            PropertySpec{.name = StandardShaderPropertyNames::
                                             INPUT_PROJECTION,
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
                        position_view = mat_proj * mat_view * position_world;
                    )"}});

        auto p_viewNormal = root.getComponent<ShaderSnippetKeeper>().new_asset(
            {ShaderSnippet::StringParams{
                .inputSpecs =
                    {
                        PropertySpec{
                            .name = StandardShaderPropertyNames::INPUT_MODEL,
                            .typeInfo =
                                StandardShaderPropertyTypes::INPUT_MODEL},
                        PropertySpec{
                            .name = StandardShaderPropertyNames::INPUT_VIEW,
                            .typeInfo =
                                StandardShaderPropertyTypes::INPUT_VIEW},
                        PropertySpec{
                            .name =
                                StandardShaderPropertyNames::INPUT_PROJECTION,
                            .typeInfo =
                                StandardShaderPropertyTypes::INPUT_PROJECTION},
                        PropertySpec{.name = StandardVertexBufferNames::NORMAL,
                                     .typeInfo =
                                         Variant::getTypeInfo<glm::vec3>()},
                    },
                .outputSpecs = {PropertySpec{
                    .name = "normal_view",
                    .typeInfo = Variant::getTypeInfo<glm::vec3>()}},
                .snippet = R"(
                    mat4 mat_viewproj = mat_proj * mat_view * mat_model;
                    vec4 origin_h = mat_viewproj * vec4(0.0, 0.0, 0.0, 1.0);
                    vec4 normal_h = mat_viewproj * vec4(normal, 1.0);
                    normal_view = normalize(mat3(mat_viewproj) * normal);
                )"}});

        p_vertexMethod =
            dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
                .tasks = {p_worldPosition, p_viewPosition, p_viewNormal},
                .friendlyName = "Classic"});

        /*
        FRAGMENT/GENERIC SHADING
        */

        auto p_shadeAmbient =
            root.getComponent<ShaderSnippetKeeper>().new_asset(
                {ShaderSnippet::StringParams{
                    .inputSpecs =
                        {
                            PropertySpec{.name = "light_color_ambient",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                        },
                    .outputSpecs =
                        {
                            PropertySpec{.name = "shade_ambient",
                                         .typeInfo =
                                             Variant::getTypeInfo<glm::vec3>()},
                        },
                    .snippet = R"(
                    shade_ambient = light_color_ambient;
                )"}});

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
                                .name = ShaderModePropertyNames::PHONG_SHADE,
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

        p_genericShaderMethod =
            dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
                .tasks = {p_shadeAmbient, p_shadeDiffuse, p_shadeSpecular, p_phongCombine},
                .friendlyName = "Classic"});

        /*
        COMPOSING
        */

        // camera matrices extractor
        auto p_extractCameraProperties =
            dynasma::makeStandalone<ComposeFunction>(ComposeFunction::SetupParams{
                .inputSpecs = {{PropertySpec{
                    .name = "scene", .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<Scene>>()}}},
                .outputSpecs = {{
                    PropertySpec{.name = StandardShaderPropertyNames::INPUT_VIEW,
                                 .typeInfo = Variant::getTypeInfo<glm::mat4>()},
                    PropertySpec{.name = StandardShaderPropertyNames::INPUT_PROJECTION,
                                 .typeInfo = Variant::getTypeInfo<glm::mat4>()},
                    PropertySpec{.name = "camera_position",
                                 .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                }},
                .p_function =
                    [](const RenderRunContext &context) {
                        try {
                            auto p_scene =
                                context.properties.get("scene").get<dynasma::FirmPtr<Scene>>();
                            auto p_windowFrame =
                                context.properties.get(StandardCompositorOutputNames::OUTPUT)
                                    .get<dynasma::FirmPtr<FrameStore>>();
                            context.properties.set(StandardShaderPropertyNames::INPUT_VIEW,
                                                   p_scene->camera.getViewMatrix());
                            context.properties.set(
                                StandardShaderPropertyNames::INPUT_PROJECTION,
                                p_scene->camera.getPerspectiveMatrix(p_windowFrame->getSize().x,
                                                                     p_windowFrame->getSize().y));
                            context.properties.set("camera_position", p_scene->camera.position);
                        }
                        catch (const std::out_of_range &e) {
                        }
                    },
                .friendlyName = "Camera properties",
            });

        p_composeMethod =
            dynasma::makeStandalone<Method<ComposeTask>>(Method<ComposeTask>::MethodParams{
                .tasks = {p_extractCameraProperties}, .friendlyName = "Classic"});
    }
};