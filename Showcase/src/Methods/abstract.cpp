#include "abstract.hpp"

#include "Vitrae/Assets/Mesh.hpp"
#include "Vitrae/Visuals/Scene.hpp"

#include <set>

MethodCollection::MethodCollection(ComponentRoot &root)
    : root(root), p_vertexMethod(dynasma::makeStandalone<Method<ShaderTask>>(
                      Method<ShaderTask>::MethodParams{})),
      p_fragmentMethod(
          dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{})),
      p_computeMethod(
          dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{})),
      p_composeMethod(
          dynasma::makeStandalone<Method<ComposeTask>>(Method<ComposeTask>::MethodParams{}))
{}
