#pragma once

#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Util/ScopedDict.hpp"

#include <functional>
#include <vector>

namespace Vitrae
{

class OpenGLRenderer;

class OpenGLComposeSceneRender : public ComposeSceneRender
{
  public:
    using ComposeSceneRender::ComposeSceneRender;

    OpenGLComposeSceneRender(const SetupParams &params);

    const std::map<StringId, PropertySpec> &getInputSpecs(
        const RenderSetupContext &args) const override;
    const std::map<StringId, PropertySpec> &getOutputSpecs() const override;

    void run(RenderRunContext args) const override;
    void prepareRequiredLocalAssets(
        std::map<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
        std::map<StringId, dynasma::FirmPtr<Texture>> &textures) const override;

  protected:
    ComponentRoot &m_root;
    String m_viewPositionOutputPropertyName;
    StringId m_sceneInputNameId, m_displayOutputNameId;
    std::optional<StringId> m_displayInputNameId;

    CullingMode m_cullingMode;
    RasterizingMode m_rasterizingMode;
    bool m_smoothFilling, m_smoothTracing, m_smoothDotting;
};

} // namespace Vitrae