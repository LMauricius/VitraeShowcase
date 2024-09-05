#pragma once

#include <iostream>

#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Pipelines/Compositing/Task.hpp"
#include "Vitrae/Pipelines/Shading/Task.hpp"
#include "Vitrae/Visuals/Compositor.hpp"

using namespace Vitrae;

struct MethodCollection
{
    ComponentRoot &root;
    dynasma::FirmPtr<Method<ShaderTask>> p_genericShaderMethod;
    dynasma::FirmPtr<Method<ShaderTask>> p_vertexMethod;
    dynasma::FirmPtr<Method<ShaderTask>> p_fragmentMethod;
    dynasma::FirmPtr<Method<ShaderTask>> p_computeMethod;
    dynasma::FirmPtr<Method<ComposeTask>> p_composeMethod;
    std::vector<std::function<void(ComponentRoot &, Renderer &, ScopedDict &)>> setupFunctions;

    MethodCollection(ComponentRoot &root);
    virtual ~MethodCollection() = default;
};