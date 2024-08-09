#pragma once

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Pipelines/Method.hpp"
#include "Vitrae/Pipelines/Shading/Task.hpp"
#include "Vitrae/Pipelines/Task.hpp"
#include "Vitrae/Util/ScopedDict.hpp"

namespace Vitrae
{
class Renderer;
class FrameStore;
class Texture;

struct RenderSetupContext
{
    Renderer &renderer;
    dynasma::FirmPtr<Method<ShaderTask>> p_defaultVertexMethod, p_defaultFragmentMethod;
};

struct RenderRunContext
{
    ScopedDict &properties;
    Renderer &renderer;
    MethodCombinator<ShaderTask> &methodCombinator;
    dynasma::FirmPtr<Method<ShaderTask>> p_defaultVertexMethod, p_defaultFragmentMethod;
    const std::map<StringId, dynasma::FirmPtr<FrameStore>> &preparedCompositorFrameStores;
    const std::map<StringId, dynasma::FirmPtr<Texture>> &preparedCompositorTextures;
};

class ComposeTask : public Task
{
  protected:
  public:
    using IOSpecsDeducingContext = RenderSetupContext;

    using Task::Task;

    virtual void run(RenderRunContext args) const = 0;
    virtual void prepareRequiredLocalAssets(
        std::map<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
        std::map<StringId, dynasma::FirmPtr<Texture>> &textures) const = 0;

    /// TODO: implement this and move to sources

    std::size_t memory_cost() const override { return 1; }

    virtual std::map<StringId, PropertySpec> &getInputSpecs(const RenderSetupContext &args)
    {
        return m_inputSpecs;
    }
    virtual std::map<StringId, PropertySpec> &getOutputSpecs(const RenderSetupContext &args)
    {
        return m_outputSpecs;
    }

    inline void extractUsedTypes(std::set<const TypeInfo *> &typeSet) const override
    {
        for (auto &specs : {m_inputSpecs, m_outputSpecs}) {
            for (auto [nameId, spec] : specs) {
                typeSet.insert(&spec.typeInfo);
            }
        }
    }
    inline void extractSubTasks(std::set<const Task *> &taskSet) const override
    {
        taskSet.insert(this);
    }
};

namespace StandardCompositorOutputNames
{
static constexpr const char OUTPUT[] = "rendered_scene";
} // namespace StandardComposeOutputNames

namespace StandardCompositorOutputTypes
{
static constexpr const TypeInfo &OUTPUT_TYPE = Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>();
}

} // namespace Vitrae