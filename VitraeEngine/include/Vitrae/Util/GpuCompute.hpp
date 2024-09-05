#pragma once

#include "Vitrae/Util/PropertyGetter.hpp"

namespace Vitrae
{

inline constexpr std::size_t GROUP_SIZE_AUTO = 0;

struct GpuComputeSetupParams
{
    PropertyGetter<std::size_t> invocationCountX;
    PropertyGetter<std::size_t> invocationCountY = 1;
    PropertyGetter<std::size_t> invocationCountZ = 1;
    PropertyGetter<std::size_t> groupSizeX = GROUP_SIZE_AUTO;
    PropertyGetter<std::size_t> groupSizeY = GROUP_SIZE_AUTO;
    PropertyGetter<std::size_t> groupSizeZ = GROUP_SIZE_AUTO;
    bool allowOutOfBoundsCompute = false;
};

} // namespace Vitrae