#pragma once

#include "Vitrae/Pipelines/Shading/Task.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <variant>

namespace Vitrae
{

class ShaderConstant : public virtual ShaderTask
{
  public:
    struct SetupParams
    {
        PropertySpec outputSpec;
        Variant value;
    };

    inline ShaderConstant(const SetupParams &params)
        : ShaderTask({}, {{params.outputSpec.name, params.outputSpec}}),
          m_outputNameId(params.outputSpec.name), m_outputSpec(params.outputSpec),
          m_value(params.value)
    {}

  protected:
    StringId m_outputNameId;
    PropertySpec m_outputSpec;
    Variant m_value;
};

struct ShaderConstantKeeperSeed
{
    using Asset = ShaderConstant;
    std::variant<ShaderConstant::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ShaderConstantKeeper = dynasma::AbstractKeeper<ShaderConstantKeeperSeed>;

} // namespace Vitrae