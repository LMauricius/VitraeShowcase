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
        inline const ParamSpec color_diffuse     = {"color_diffuse",  TYPE_INFO<glm::vec4>};
        inline const ParamSpec color_normal      = {"color_normal",  TYPE_INFO<glm::vec4>};
        inline const ParamSpec color_specular    = {"color_specular",  TYPE_INFO<glm::vec4>};
        inline const ParamSpec color_shininess   = {"color_shininess",  TYPE_INFO<glm::vec4>};
        inline const ParamSpec color_emissive    = {"color_emissive",  TYPE_INFO<glm::vec4>};

        inline const ParamSpec color_base        = {"color_base",  TYPE_INFO<glm::vec4>};
        inline const ParamSpec color_metalness   = {"color_metalness",  TYPE_INFO<glm::vec4>};
        inline const ParamSpec color_smoothness  = {"color_smoothness",  TYPE_INFO<glm::vec4>};

        // clang-format on

    } // namespace StandardParam
} // namespace Vitrae