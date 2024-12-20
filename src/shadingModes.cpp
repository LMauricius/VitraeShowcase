#include "shadingModes.hpp"

#include "Vitrae/Pipelines/Shading/Snippet.hpp"

#include "dynasma/standalone.hpp"

ShadingModeSetter::ShadingModeSetter(ComponentRoot &root)
{
    /*
    PHONG
    */

    // phong vertex shading uses the default method
    p_phong_vertexMethod =
        dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{});

    // Phong method takes an input called "phong_shade" and passes it to the fragment output
    p_phong_fragmentMethod = dynasma::makeStandalone<
        Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
        .tasks = {root.getComponent<ShaderSnippetKeeper>().new_asset(
            {ShaderSnippet::StringParams{
                .inputSpecs =
                    {
                        PropertySpec{
                            .name = ShaderModePropertyNames::PHONG_SHADE,
                            .typeInfo =
                                StandardShaderPropertyTypes::FRAGMENT_OUTPUT},
                    },
                .outputSpecs = {PropertySpec{
                    .name = StandardShaderPropertyNames::FRAGMENT_OUTPUT,
                    .typeInfo = StandardShaderPropertyTypes::FRAGMENT_OUTPUT}},
                .snippet = R"(
                    shade = phong_shade;
                )",
            }})},
        .fallbackMethods = {},
        .friendlyName = "Phong"});
}

void ShadingModeSetter::setModes(ComponentRoot &root)
{
    root.addAiMaterialShadingInfo(
        aiShadingMode_Phong,
        Vitrae::ComponentRoot::AiMaterialShadingInfo{.vertexMethod = p_phong_vertexMethod,
                                                     .fragmentMethod = p_phong_fragmentMethod});
}
