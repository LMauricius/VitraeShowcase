#pragma once

#include "../shadingModes.hpp"
#include "abstract.hpp"

#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/FrameToTexture.hpp"
#include "Vitrae/Pipelines/Compositing/Function.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Shading/Constant.hpp"
#include "Vitrae/Pipelines/Shading/Function.hpp"

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
            root.getComponent<ShaderFunctionKeeper>().new_asset({ShaderFunction::StringParams{
                .inputSpecs = {PropertySpec{.name = StandardShaderPropertyNames::INPUT_MODEL,
                                            .typeInfo = StandardShaderPropertyTypes::INPUT_MODEL},
                               PropertySpec{.name = StandardVertexBufferNames::POSITION,
                                            .typeInfo = Variant::getTypeInfo<glm::vec3>()}},
                .outputSpecs = {PropertySpec{.name = "position_world",
                                             .typeInfo = Variant::getTypeInfo<glm::vec4>()}},
                .snippet = R"(
                    void vertexWorldPosition(
                        in mat4 mat_model, in vec3 position_mesh,
                        out vec4 position_world
                    ) {
                        position_world = mat_model * vec4(position_mesh, 1.0);
                    }
                )",
                .functionName = "vertexWorldPosition"}});

        auto p_viewPosition =
            root.getComponent<ShaderFunctionKeeper>().new_asset({ShaderFunction::StringParams{
                .inputSpecs =
                    {
                        PropertySpec{.name = StandardShaderPropertyNames::INPUT_VIEW,
                                     .typeInfo = StandardShaderPropertyTypes::INPUT_VIEW},
                        PropertySpec{.name = StandardShaderPropertyNames::INPUT_PROJECTION,
                                     .typeInfo = StandardShaderPropertyTypes::INPUT_PROJECTION},
                        PropertySpec{.name = "position_world",
                                     .typeInfo = Variant::getTypeInfo<glm::vec4>()},
                    },
                .outputSpecs = {PropertySpec{.name = "position_view",
                                             .typeInfo =
                                                 StandardShaderPropertyTypes::VERTEX_OUTPUT}},
                .snippet = R"(
                    void vertexViewPosition(
                        in mat4 mat_view, in mat4 mat_proj, in vec4 position_world,
                        out vec4 position_view
                    ) {
                        position_view = mat_proj * mat_view * position_world;
                    }
                )",
                .functionName = "vertexViewPosition"}});

        p_vertexMethod =
            dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
                .tasks = {p_worldPosition, p_viewPosition}, .friendlyName = "Classic"});

        /*
        FRAGMENT SHADING
        */

        auto p_shadeAmbient =
            root.getComponent<ShaderFunctionKeeper>().new_asset({ShaderFunction::StringParams{
                .inputSpecs =
                    {
                        PropertySpec{.name = "light_color_ambient",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                    },
                .outputSpecs =
                    {
                        PropertySpec{.name = "shade_ambient",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                    },
                .snippet = R"(
                    void setAmbient(
                        in vec3 light_color_ambient,
                        out vec3 shade_ambient
                    ) {
                        shade_ambient = light_color_ambient;
                    }
                )",
                .functionName = "setAmbient"}});

        auto p_shadeDiffuse =
            root.getComponent<ShaderFunctionKeeper>().new_asset({ShaderFunction::StringParams{
                .inputSpecs =
                    {
                        PropertySpec{.name = "normal",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                        PropertySpec{.name = "light_direction",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                        PropertySpec{.name = "light_color_primary",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                        PropertySpec{.name = "light_shadow_factor",
                                     .typeInfo = Variant::getTypeInfo<float>()},
                    },
                .outputSpecs =
                    {
                        PropertySpec{.name = "shade_diffuse",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                    },
                .snippet = R"(
                    void lightDiffuse(
                        in vec3 normal, in vec3 light_direction,
                        in vec3 light_color_primary, in float light_shadow_factor,
                        out vec3 shade_diffuse
                    ) {
                        shade_diffuse = max(0.0, -dot(light_direction, normal)) * light_color_primary * light_shadow_factor;
                    }
                )",
                .functionName = "lightDiffuse"}});

        auto
            p_shadeSpecular =
                root.getComponent<ShaderFunctionKeeper>()
                    .new_asset(
                        {ShaderFunction::StringParams{
                            .inputSpecs =
                                {
                                    PropertySpec{.name = "camera_position",
                                                 .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                                    PropertySpec{.name = "position_world",
                                                 .typeInfo = Variant::getTypeInfo<glm::vec4>()},
                                    PropertySpec{
                                        .name = StandardMaterialTextureNames::SPECULAR,
                                        .typeInfo =
                                            Variant::getTypeInfo<dynasma::FirmPtr<Texture>>()},
                                    PropertySpec{.name = StandardVertexBufferNames::TEXTURE_COORD,
                                                 .typeInfo = Variant::getTypeInfo<glm::vec2>()},
                                    PropertySpec{
                                        .name = StandardMaterialPropertyNames::COL_SPECULAR,
                                        .typeInfo = StandardMaterialPropertyTypes::COL_SPECULAR},
                                    PropertySpec{
                                        .name = StandardMaterialPropertyNames::SHININESS,
                                        .typeInfo = StandardMaterialPropertyTypes::SHININESS},
                                    PropertySpec{.name = "normal",
                                                 .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                                    PropertySpec{.name = "light_direction",
                                                 .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                                    PropertySpec{.name = "light_color_primary",
                                                 .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                                    PropertySpec{.name = "light_shadow_factor",
                                                 .typeInfo = Variant::getTypeInfo<float>()},
                                },
                            .outputSpecs =
                                {
                                    PropertySpec{.name = "shade_specular",
                                                 .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                                },
                            .snippet = R"(
                    void lightSpecular(
                        in vec3 camera_position, in vec4 position_world,
                        in sampler2D tex_specular, in vec2 tex_coord, in vec4 color_specular, in float shininess,
                        in vec3 normal, in vec3 light_direction,
                        in vec3 light_color_primary, in float light_shadow_factor,
                        out vec3 shade_specular
                    ) {
                        vec4 color_specular_tot = texture2D(tex_specular, tex_coord);
                        vec3 dirToEye = normalize(camera_position - position_world.xyz);
                        vec3 reflRay = 2 * dot(-light_direction, normal) * normal + light_direction;
                        shade_specular =
                            pow(max(dot(reflRay, dirToEye), 0.001), shininess) * light_shadow_factor *
                            color_specular_tot.rgb * light_color_primary;
                    }
                )",
                            .functionName = "lightSpecular"}});

        auto p_phongCombine =
            root.getComponent<ShaderFunctionKeeper>()
                .new_asset(
                    {ShaderFunction::StringParams{
                        .inputSpecs =
                            {
                                PropertySpec{.name = "shade_diffuse",
                                             .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                                PropertySpec{.name = "shade_specular",
                                             .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                                PropertySpec{.name = "shade_ambient",
                                             .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                                PropertySpec{.name = StandardMaterialTextureNames::DIFFUSE,
                                             .typeInfo =
                                                 Variant::getTypeInfo<dynasma::FirmPtr<Texture>>()},
                                PropertySpec{.name = StandardVertexBufferNames::TEXTURE_COORD,
                                             .typeInfo = Variant::getTypeInfo<glm::vec2>()},
                            },
                        .outputSpecs =
                            {
                                PropertySpec{
                                    .name = ShaderModePropertyNames::PHONG_SHADE,
                                    .typeInfo = StandardShaderPropertyTypes::FRAGMENT_OUTPUT},
                            },
                        .snippet = R"(
                    void phongCombine(
                        in vec3 shade_diffuse, in vec3 shade_specular, in vec3 shade_ambient,
                        in sampler2D tex_diffuse,
                        in vec2 tex_coord,
                        out vec4 phong_shade
                    ) {
                        vec4 color_diffuse = texture2D(tex_diffuse, tex_coord);
                        phong_shade = vec4(
                            color_diffuse.rgb * (shade_diffuse + shade_ambient) +
                            shade_specular, 
                            color_diffuse.a);
                    }
                )",
                        .functionName = "phongCombine"}});

        p_fragmentMethod =
            dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
                .tasks = {p_shadeAmbient, p_shadeDiffuse, p_shadeSpecular, p_phongCombine},
                .friendlyName = "Classic"});

        /*
        COMPOSING
        */

        // camera matrices extractor
        auto p_extractCameraProperties = dynasma::makeStandalone<
            ComposeFunction>(ComposeFunction::SetupParams{
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
            .p_function = [](const RenderRunContext &context) {
                try {
                    auto p_scene = context.properties.get("scene").get<dynasma::FirmPtr<Scene>>();
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
            }});

        // normal render

        auto p_clear = root.getComponent<ComposeClearRenderKeeper>().new_asset(
            {ComposeClearRender::SetupParams{.root = root,
                                             .backgroundColor = glm::vec4(0.0f, 0.5f, 0.8f, 1.0f),
                                             .displayOutputPropertyName = "display_cleared"}});

        auto p_normalRender = root.getComponent<ComposeSceneRenderKeeper>().new_asset(
            {ComposeSceneRender::SetupParams{
                .root = root,
                .sceneInputPropertyName = "scene",
                .vertexPositionOutputPropertyName = "position_view",
                .displayInputPropertyName = "display_cleared",
                .displayOutputPropertyName = StandardCompositorOutputNames::OUTPUT,
            }});

        p_composeMethod =
            dynasma::makeStandalone<Method<ComposeTask>>(Method<ComposeTask>::MethodParams{
                .tasks = {p_extractCameraProperties, p_clear, p_normalRender},
                .friendlyName = "Classic"});
    }
};