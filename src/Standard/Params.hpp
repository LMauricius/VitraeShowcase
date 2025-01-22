#pragma once

#include "Vitrae/Data/LevelOfDetail.hpp"
#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Params/ParamSpec.hpp"
#include "dynasma/pointer.hpp"

namespace VitraeCommon
{
    using namespace Vitrae;

    /**
     * Namespace containing commonly used ParamSpecs, identifiable by their string names
     */
    namespace StandardParam
    {
        using namespace Vitrae::StandardParam;

        // clang-format off

        /// @subsection Vertex properties
        inline const ParamSpec tangent  = {"tangent",  TYPE_INFO<glm::vec3>};
        inline const ParamSpec bitangent = {"bitangent", TYPE_INFO<glm::vec3>};

        // clang-format on

    } // namespace StandardParam
} // namespace Vitrae