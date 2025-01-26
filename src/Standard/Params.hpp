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
    namespace StandardParam
    {
        using namespace Vitrae::StandardParam;

        // clang-format off

        /// @subsection Vertex properties
        inline const ParamSpec tangent  = {"tangent",  TYPE_INFO<glm::vec3>};
        inline const ParamSpec bitangent = {"bitangent", TYPE_INFO<glm::vec3>};

        /// @subsection Textures
        inline const ParamSpec tex_base        = {"tex_base",  TYPE_INFO<dynasma::FirmPtr<Texture>>};
        inline const ParamSpec tex_metalness   = {"tex_metalness",  TYPE_INFO<dynasma::FirmPtr<Texture>>};
        inline const ParamSpec tex_smoothness  = {"tex_smoothness",  TYPE_INFO<dynasma::FirmPtr<Texture>>};

        inline const ParamSpec base        = {"base",  TYPE_INFO<glm::vec3>};
        inline const ParamSpec metalness   = {"metalness",  TYPE_INFO<glm::vec1>};
        inline const ParamSpec smoothness  = {"smoothness",  TYPE_INFO<glm::vec1>};

        // clang-format on

    } // namespace StandardParam
} // namespace Vitrae