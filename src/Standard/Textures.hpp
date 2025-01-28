#pragma once

#include "Vitrae/Data/LevelOfDetail.hpp"
#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Params/ParamSpec.hpp"
#include "Vitrae/Params/Standard.hpp"
#include "dynasma/pointer.hpp"

namespace VitraeCommon
{
using namespace Vitrae;

/**
 * Namespace containing commonly used ParamSpecs, identifiable by their string names
 */
namespace StandardTexture
{
// clang-format off
        inline const char diffuse[]    = "diffuse";
        inline const char normal[]     = "normal";
        inline const char specular[]   = "specular";
        inline const char emissive[]   = "emissive";

        inline const char base[]       = "base";
        inline const char metalness[]  = "metalness";
        inline const char smoothness[] = "smoothness";

// clang-format on

} // namespace StandardTexture
} // namespace VitraeCommon