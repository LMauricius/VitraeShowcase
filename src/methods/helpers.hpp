#pragma once

#include "Vitrae/Assets/Mesh.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/Assets/Texture.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"

#include "dynasma/standalone.hpp"

namespace VitraeCommon
{
    using namespace Vitrae;

    inline bool isOpaque(const Material &mat)
    {
        auto &matProperties = mat.getProperties();
        if (auto it_tex_diffuse = matProperties.find("diffuse"); it_tex_diffuse != matProperties.end())
        {
            auto p_tex_diffuse = (*it_tex_diffuse).second.get<dynasma::FirmPtr<Texture>>();

            if (p_tex_diffuse->getStats().has_value() && p_tex_diffuse->getStats().value().averageColor.a < 1.0f)
            {
                return false;
            }
        }
        return true;
    }
}