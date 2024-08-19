#include "Vitrae/Pipelines/Compositing/Function.hpp"

#include "MMeter.h"

namespace Vitrae
{

ComposeFunction::ComposeFunction(const SetupParams &params)
    : ComposeTask(params.inputSpecs, params.outputSpecs), mp_function(params.p_function)
{}

void ComposeFunction::run(RenderRunContext args) const
{
    MMETER_SCOPE_PROFILER("ComposeFunction::run");

    mp_function(args);
}

void ComposeFunction::prepareRequiredLocalAssets(
    std::map<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
    std::map<StringId, dynasma::FirmPtr<Texture>> &textures, const ScopedDict &properties) const
{}

} // namespace Vitrae