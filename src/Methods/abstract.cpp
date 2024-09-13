#include "abstract.hpp"

#include "Vitrae/Assets/Mesh.hpp"
#include "Vitrae/Visuals/Scene.hpp"

#include <set>

MethodCollection::MethodCollection(ComponentRoot &root)
    : root(root), p_genericShaderMethod(dynasma::makeStandalone<Method<ShaderTask>>()),
      p_vertexMethod(dynasma::makeStandalone<Method<ShaderTask>>()),
      p_fragmentMethod(dynasma::makeStandalone<Method<ShaderTask>>()),
      p_computeMethod(dynasma::makeStandalone<Method<ShaderTask>>()),
      p_composeMethod(dynasma::makeStandalone<Method<ComposeTask>>())
{}
