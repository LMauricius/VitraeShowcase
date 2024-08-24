#include "Vitrae/Pipelines/Compositing/Function.hpp"

#include "MMeter.h"

namespace Vitrae
{

ComposeFunction::ComposeFunction(const SetupParams &params)
    : ComposeTask(params.inputSpecs, params.outputSpecs), mp_function(params.p_function)
{
    for (auto &spec : params.outputSpecs) {
        if (m_friendlyName.size()) {
            m_friendlyName += String("\n+ ");
        }
        m_friendlyName += spec.name;
    }
}

void ComposeFunction::run(RenderRunContext args) const
{
    MMETER_SCOPE_PROFILER("ComposeFunction::run");

    mp_function(args);
}

void ComposeFunction::prepareRequiredLocalAssets(
    StableMap<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
    StableMap<StringId, dynasma::FirmPtr<Texture>> &textures, const ScopedDict &properties) const
{}

StringView ComposeFunction::getFriendlyName() const
{
    return m_friendlyName;
}

} // namespace Vitrae