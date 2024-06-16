#include "Vitrae/Renderers/OpenGL/Compositing/ClearRender.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Renderers/OpenGL/FrameStore.hpp"
#include "Vitrae/Renderers/OpenGL/Mesh.hpp"
#include "Vitrae/Renderers/OpenGL/ShaderCompilation.hpp"
#include "Vitrae/Renderers/OpenGL/Texture.hpp"
#include "Vitrae/Util/Property.hpp"

namespace Vitrae
{

OpenGLComposeClearRender::OpenGLComposeClearRender(const SetupParams &params)
    : ComposeClearRender(
          params.displayInputPropertyName.empty()
              ? std::span<const PropertySpec>{}
              : std::span<const PropertySpec>{{PropertySpec{
                    .name = params.displayInputPropertyName,
                    .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>()}}},
          std::span<const PropertySpec>{
              {PropertySpec{.name = params.displayOutputPropertyName,
                            .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>()}}}),
      m_root(params.root), m_displayInputNameId(params.displayInputPropertyName.empty()
                                                    ? std::optional<StringId>()
                                                    : params.displayInputPropertyName),
      m_displayOutputNameId(params.displayOutputPropertyName), m_color(params.backgroundColor)
{}

void OpenGLComposeClearRender::run(RenderRunContext args) const
{
    OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(m_root.getComponent<Renderer>());
    CompiledGLSLShaderCacher &shaderCacher = m_root.getComponent<CompiledGLSLShaderCacher>();

    OpenGLFrameStore &frame = static_cast<OpenGLFrameStore &>(
        *args.properties.get(m_displayOutputNameId).get<dynasma::FirmPtr<FrameStore>>());

    frame.enterRender({0.0f, 0.0f}, {1.0f, 1.0f});

    glClearColor(m_color.r, m_color.g, m_color.b, m_color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    frame.exitRender();
}

void OpenGLComposeClearRender::prepareRequiredLocalAssets(
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