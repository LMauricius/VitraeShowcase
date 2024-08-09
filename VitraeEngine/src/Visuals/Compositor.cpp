#include "Vitrae/Visuals/Compositor.hpp"
#include "Vitrae/ComponentRoot.hpp"

#include <ranges>

namespace Vitrae
{
class Renderer;
Compositor::Compositor(ComponentRoot &root) : m_root(root), m_pipeline() {}

Compositor::Compositor(ComponentRoot &root, dynasma::FirmPtr<Method<ComposeTask>> p_method,
                       dynasma::FirmPtr<FrameStore> p_output)
    : m_root(root), m_needsRebuild(false), m_needsFrameStoreRegeneration(false),
      m_pipeline(p_method, {},
                 RenderSetupContext{
                     .renderer = m_root.getComponent<Renderer>(),
                     .p_defaultVertexMethod = m_defaultVertexMethod,
                     .p_defaultFragmentMethod = m_defaultFragmentMethod,
                 })
{
    m_preparedFrameStores[StandardCompositorOutputNames::OUTPUT] = p_output;

    // here we actually setup the pipeline
    setComposeMethod(p_method);
}
std::size_t Compositor::memory_cost() const
{
    /// TODO: implement
    return sizeof(Compositor);
}

void Compositor::setComposeMethod(dynasma::FirmPtr<Method<ComposeTask>> p_method)
{
    mp_composeMethod = p_method;
    m_needsRebuild = true;
}

void Compositor::setDefaultShadingMethod(dynasma::FirmPtr<Method<ShaderTask>> p_vertexMethod,
                                         dynasma::FirmPtr<Method<ShaderTask>> p_fragmentMethod)
{
    m_defaultVertexMethod = p_vertexMethod;
    m_defaultFragmentMethod = p_fragmentMethod;
}

void Compositor::setOutput(dynasma::FirmPtr<FrameStore> p_store)
{
    m_preparedFrameStores[StandardCompositorOutputNames::OUTPUT] = p_store;

    m_needsFrameStoreRegeneration = true;
}

void Compositor::compose()
{
    ScopedDict localVars(&parameters);

    // set the output frame property
    localVars.set(StandardCompositorOutputNames::OUTPUT,
                  m_preparedFrameStores.at(StandardCompositorOutputNames::OUTPUT));

    // setup the rendering context
    Renderer &rend = m_root.getComponent<Renderer>();
    RenderRunContext context{.properties = localVars,
                             .renderer = rend,
                             .methodCombinator = m_shadingMethodCombinator,
                             .p_defaultVertexMethod = m_defaultVertexMethod,
                             .p_defaultFragmentMethod = m_defaultFragmentMethod,
                             .preparedCompositorFrameStores = m_preparedFrameStores,
                             .preparedCompositorTextures = m_preparedTextures};

    bool tryExecute = true;

    while (tryExecute) {
        tryExecute = false;

        // rebuild the pipeline if needed
        if (m_needsRebuild) {
            rebuildPipeline();
        }
        if (m_needsFrameStoreRegeneration) {
            regenerateFrameStores();
        }

        try {
            // execute the pipeline
            for (auto &pipeitem : m_pipeline.items) {
                pipeitem.p_task->run(context);
            }

            // sync the framebuffers
            std::set<dynasma::FirmPtr<FrameStore>> uniqueFrameStores;
            for (auto [nameId, p_store] : m_preparedFrameStores) {
                uniqueFrameStores.insert(p_store);
            }
            for (auto p_store : uniqueFrameStores) {
                p_store->sync();
            }
        }
        catch (ComposeTaskRequirementsChangedException) {
            m_needsRebuild = true;
            tryExecute = true;
        }
    }
}

void Compositor::rebuildPipeline()
{
    m_needsRebuild = false;

    RenderSetupContext context{
        .renderer = m_root.getComponent<Renderer>(),
        .p_defaultVertexMethod = m_defaultVertexMethod,
        .p_defaultFragmentMethod = m_defaultFragmentMethod,
    };

    m_pipeline = Pipeline<ComposeTask>(
        mp_composeMethod,
        {{PropertySpec{.name = StandardCompositorOutputNames::OUTPUT,
                       .typeInfo = StandardCompositorOutputTypes::OUTPUT_TYPE}}},
        context);

    m_needsFrameStoreRegeneration = true;
}

void Compositor::regenerateFrameStores()
{
    m_needsFrameStoreRegeneration = false;

    // clear the buffers (except for the final output)
    auto p_finalFrame = m_preparedFrameStores[StandardCompositorOutputNames::OUTPUT];
    m_preparedFrameStores.clear();
    m_preparedTextures.clear();
    m_preparedFrameStores[StandardCompositorOutputNames::OUTPUT] = p_finalFrame;

    // fill the buffers
    for (auto &pipeitem : std::ranges::reverse_view{m_pipeline.items}) {
        pipeitem.p_task->prepareRequiredLocalAssets(m_preparedFrameStores, m_preparedTextures);
    }
}

} // namespace Vitrae