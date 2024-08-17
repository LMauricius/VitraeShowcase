#include "Vitrae/Renderers/OpenGL/Compositing/SceneRender.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Renderers/OpenGL/FrameStore.hpp"
#include "Vitrae/Renderers/OpenGL/Mesh.hpp"
#include "Vitrae/Renderers/OpenGL/ShaderCompilation.hpp"
#include "Vitrae/Renderers/OpenGL/Texture.hpp"
#include "Vitrae/Util/Variant.hpp"
#include "Vitrae/Visuals/Scene.hpp"

namespace Vitrae
{

OpenGLComposeSceneRender::OpenGLComposeSceneRender(const SetupParams &params)
    : ComposeSceneRender(
          std::span<const PropertySpec>{{
              PropertySpec{.name = params.sceneInputPropertyName,
                           .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<Scene>>()},
          }},
          std::span<const PropertySpec>{
              {PropertySpec{.name = params.displayOutputPropertyName,
                            .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>()}}}),
      m_root(params.root),
      m_viewPositionOutputPropertyName(params.vertexPositionOutputPropertyName),
      m_sceneInputNameId(params.sceneInputPropertyName),
      m_displayInputNameId(params.displayInputPropertyName.empty()
                               ? std::optional<StringId>()
                               : params.displayInputPropertyName),
      m_displayOutputNameId(params.displayOutputPropertyName), m_cullingMode(params.cullingMode),
      m_rasterizingMode(params.rasterizingMode), m_smoothFilling(params.smoothFilling),
      m_smoothTracing(params.smoothTracing), m_smoothDotting(params.smoothDotting)
{

    /*
    We reuse m_inputSpecs as container of the scene and display input specs,
    but we will actually return the renderer's cached input specs for this task
    */

    if (!params.displayInputPropertyName.empty()) {
        m_inputSpecs.emplace(
            params.displayInputPropertyName,
            PropertySpec{.name = params.displayInputPropertyName,
                         .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>()});
    }
}

const std::map<StringId, PropertySpec> &OpenGLComposeSceneRender::getInputSpecs(
    const RenderSetupContext &args) const
{
    OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(args.renderer);
    return rend.getSceneRenderInputDependencies(this, args.p_defaultVertexMethod,
                                                args.p_defaultFragmentMethod);
}

const std::map<StringId, PropertySpec> &OpenGLComposeSceneRender::getOutputSpecs() const
{
    return m_outputSpecs;
}

void OpenGLComposeSceneRender::run(RenderRunContext args) const
{
    OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(m_root.getComponent<Renderer>());
    CompiledGLSLShaderCacher &shaderCacher = m_root.getComponent<CompiledGLSLShaderCacher>();

    // add common inputs to renderer's cached input specs for this task
    bool needsRebuild = false;
    for (auto [nameId, spec] : m_inputSpecs) {
        needsRebuild = needsRebuild ||
                       rend.specifySceneRenderInputDependency(this, args.p_defaultVertexMethod,
                                                              args.p_defaultFragmentMethod, spec);
    }

    // fail early if we already need to rebuild
    // DO save the output, the pipeline expects this property to be set
    if (needsRebuild) {
        args.properties.set(m_displayOutputNameId,
                            args.preparedCompositorFrameStores.at(m_displayOutputNameId));

        throw ComposeTaskRequirementsChangedException();
    }

    // extract common inputs
    Scene &scene = *args.properties.get(m_sceneInputNameId).get<dynasma::FirmPtr<Scene>>();

    dynasma::FirmPtr<FrameStore> p_frame =
        m_displayInputNameId.has_value()
            ? args.properties.get(m_displayInputNameId.value()).get<dynasma::FirmPtr<FrameStore>>()
            : args.preparedCompositorFrameStores.at(m_displayOutputNameId);
    OpenGLFrameStore &frame = static_cast<OpenGLFrameStore &>(*p_frame);

    // build map of shaders to materials to mesh props
    std::map<std::pair<dynasma::FirmPtr<Method<ShaderTask>>, dynasma::FirmPtr<Method<ShaderTask>>>,
             std::map<dynasma::FirmPtr<const Material>, std::vector<const MeshProp *>>>
        methods2materials2props;

    for (auto &meshProp : scene.meshProps) {
        auto mat = meshProp.p_mesh->getMaterial().getLoaded();

        methods2materials2props[{mat->getVertexMethod(), mat->getFragmentMethod()}]
                               [meshProp.p_mesh->getMaterial()]
                                   .push_back(&meshProp);
    }

    switch (m_rasterizingMode) {
    // derivational methods (all methods for now)
    case RasterizingMode::DerivationalFillCenters:
    case RasterizingMode::DerivationalFillEdges:
    case RasterizingMode::DerivationalFillVertices:
    case RasterizingMode::DerivationalTraceEdges:
    case RasterizingMode::DerivationalTraceVertices:
    case RasterizingMode::DerivationalDotVertices: {
        frame.enterRender(args.properties, {0.0f, 0.0f}, {1.0f, 1.0f});

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        switch (m_cullingMode) {
        case CullingMode::None:
            glDisable(GL_CULL_FACE);
            break;
        case CullingMode::Backface:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
        case CullingMode::Frontface:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        }

        // smoothing
        if (m_smoothFilling) {
            glEnable(GL_POLYGON_SMOOTH);
            glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
        } else {
            glDisable(GL_POLYGON_SMOOTH);
        }
        if (m_smoothTracing) {
            glEnable(GL_LINE_SMOOTH);
            glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        } else {
            glDisable(GL_LINE_SMOOTH);
        }

        // generic function for all passes
        auto runDerivationalPass =
            [&]() {
                // render the scene
                // iterate over shaders
                for (auto &[methods, materials2props] : methods2materials2props) {
                    auto [vertexMethod, fragmentMethod] = methods;

                    // compile shader for this material
                    dynasma::FirmPtr<CompiledGLSLShader> p_compiledShader =
                        shaderCacher.retrieve_asset({CompiledGLSLShader::SurfaceShaderParams(
                            args.methodCombinator.getCombinedMethod(args.p_defaultVertexMethod,
                                                                    vertexMethod),
                            args.methodCombinator.getCombinedMethod(args.p_defaultFragmentMethod,
                                                                    fragmentMethod),
                            m_viewPositionOutputPropertyName, frame.getRenderComponents(),
                            m_root)});

                    glUseProgram(p_compiledShader->programGLName);

                    // set the 'environmental' uniforms
                    int freeBindingIndex = 0;
                    GLint glModelMatrixUniformId;
                    for (auto [propertyNameId, uniSpec] : p_compiledShader->uniformSpecs) {
                        if (propertyNameId == StandardShaderPropertyNames::INPUT_MODEL) {
                            // this is set per model
                            glModelMatrixUniformId = uniSpec.glNameId;

                        } else {
                            auto p = args.properties.getPtr(propertyNameId);
                            if (p) {
                                rend.getTypeConversion(uniSpec.srcSpec.typeInfo)
                                    .setUniform(uniSpec.glNameId, *p);
                            } else {
                                needsRebuild = needsRebuild ||
                                               rend.specifySceneRenderInputDependency(
                                                   this, args.p_defaultVertexMethod,
                                                   args.p_defaultFragmentMethod, uniSpec.srcSpec);
                            }
                        }
                    }
                    for (auto [propertyNameId, uniSpec] : p_compiledShader->bindingSpecs) {
                        auto p = args.properties.getPtr(propertyNameId);
                        if (p) {
                            rend.getTypeConversion(uniSpec.srcSpec.typeInfo)
                                .setBinding(freeBindingIndex, *p);
                            glUniform1i(uniSpec.glNameId, freeBindingIndex);
                            freeBindingIndex++;
                        } else {
                            needsRebuild =
                                needsRebuild || rend.specifySceneRenderInputDependency(
                                                    this, args.p_defaultVertexMethod,
                                                    args.p_defaultFragmentMethod, uniSpec.srcSpec);
                        }
                    }

                    // helper for material properties
                    auto setPropertyToShader = [&](StringId nameId, const Variant &value) {
                        if (auto it = p_compiledShader->uniformSpecs.find(nameId);
                            it != p_compiledShader->uniformSpecs.end()) {
                            rend.getTypeConversion(it->second.srcSpec.typeInfo)
                                .setUniform(it->second.glNameId, value);
                        } else if (auto it = p_compiledShader->bindingSpecs.find(nameId);
                                   it != p_compiledShader->bindingSpecs.end()) {
                            rend.getTypeConversion(it->second.srcSpec.typeInfo)
                                .setBinding(freeBindingIndex, value);
                            glUniform1i(it->second.glNameId, freeBindingIndex);
                            freeBindingIndex++;
                        }
                    };

                    if (!needsRebuild) {

                        // iterate over materials
                        for (auto [material, props] : materials2props) {

                            // get the textures and properties
                            for (auto [nameId, p_texture] : material->getTextures()) {
                                setPropertyToShader(nameId, p_texture);
                            }
                            for (auto [nameId, value] : material->getProperties()) {
                                setPropertyToShader(nameId, value);
                            }

                            // iterate over meshes
                            for (auto p_meshProp : props) {
                                OpenGLMesh &mesh = static_cast<OpenGLMesh &>(*p_meshProp->p_mesh);
                                mesh.loadToGPU(rend);

                                glBindVertexArray(mesh.VAO);
                                glUniformMatrix4fv(glModelMatrixUniformId, 1, GL_FALSE,
                                                   &(p_meshProp->transform.getModelMatrix()[0][0]));
                                glDrawElements(GL_TRIANGLES, 3 * mesh.getTriangles().size(),
                                               GL_UNSIGNED_INT, 0);
                            }
                        }
                    }

                    glUseProgram(0);
                }
            };

        // render filled polygons
        switch (m_rasterizingMode) {
        case RasterizingMode::DerivationalFillCenters:
        case RasterizingMode::DerivationalFillEdges:
        case RasterizingMode::DerivationalFillVertices:
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            runDerivationalPass();
            break;
        }

        // render edges
        switch (m_rasterizingMode) {
        case RasterizingMode::DerivationalFillEdges:
        case RasterizingMode::DerivationalTraceEdges:
        case RasterizingMode::DerivationalTraceVertices:
            if (m_smoothTracing) {
                glDepthMask(GL_FALSE);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glEnable(GL_BLEND);
                glLineWidth(1.5);
            } else {
                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);
                glLineWidth(1.0);
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            runDerivationalPass();
            break;
        }

        // render vertices
        switch (m_rasterizingMode) {
        case RasterizingMode::DerivationalFillVertices:
        case RasterizingMode::DerivationalTraceVertices:
        case RasterizingMode::DerivationalDotVertices:
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            runDerivationalPass();
            break;
        }

        glDepthMask(GL_TRUE);

        frame.exitRender();
        break;
    }
    }

    args.properties.set(m_displayOutputNameId, p_frame);

    if (needsRebuild) {
        throw ComposeTaskRequirementsChangedException();
    }
}

void OpenGLComposeSceneRender::prepareRequiredLocalAssets(
    std::map<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
    std::map<StringId, dynasma::FirmPtr<Texture>> &textures) const
{
    // We just need to check whether the frame store is already prepared and make it input also
    if (auto it = frameStores.find(m_displayOutputNameId); it != frameStores.end()) {
        if (m_displayInputNameId.has_value()) {
            frameStores.emplace(m_displayInputNameId.value(), it->second);
        }
    } else {
        throw std::runtime_error("Frame store not found");
    }
}

} // namespace Vitrae