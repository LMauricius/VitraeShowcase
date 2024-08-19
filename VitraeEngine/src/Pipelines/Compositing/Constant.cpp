#include "Vitrae/Pipelines/Compositing/Constant.hpp"

#include "MMeter.h"

namespace Vitrae
{
ComposeConstant::ComposeConstant(const SetupParams &params)
    : ComposeTask({}, {{params.outputSpec.name, params.outputSpec}}),
      m_outputNameId(params.outputSpec.name), m_outputSpec(params.outputSpec), m_value(params.value)
{}

void ComposeConstant::run(RenderRunContext args) const
{
    MMETER_SCOPE_PROFILER("ComposeConstant::run");

    args.properties.set(m_outputNameId, m_value);
}

void ComposeConstant::prepareRequiredLocalAssets(
    std::map<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
    std::map<StringId, dynasma::FirmPtr<Texture>> &textures, const ScopedDict &properties) const
{}

} // namespace Vitrae