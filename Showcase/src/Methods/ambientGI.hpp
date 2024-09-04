#pragma once

#include "../shadingModes.hpp"
#include "abstract.hpp"

#include "GI/Generation.hpp"
#include "GI/Probe.hpp"

#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/FrameToTexture.hpp"
#include "Vitrae/Pipelines/Compositing/Function.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Shading/Constant.hpp"
#include "Vitrae/Pipelines/Shading/Function.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"

#include "dynasma/standalone.hpp"

#include "MMeter.h"

#include <iostream>

using namespace Vitrae;

struct MethodsAmbientGI : MethodCollection
{
    inline MethodsAmbientGI(ComponentRoot &root) : MethodCollection(root)
    {
        using namespace GI;

        /*
        VERTEX SHADING
        */

        p_vertexMethod = dynasma::makeStandalone<Method<ShaderTask>>(
            Method<ShaderTask>::MethodParams{.tasks = {}});

        /*
        FRAGMENT SHADING
        */

        auto p_shadeAmbient =
            root.getComponent<ShaderFunctionKeeper>().new_asset({ShaderFunction::StringParams{
                .inputSpecs =
                    {
                        PropertySpec{.name = "gpuProbeStates",
                                     .typeInfo = Variant::getTypeInfo<ProbeStateBufferPtr>()},
                        PropertySpec{.name = "giWorldStart",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                        PropertySpec{.name = "giGridSize",
                                     .typeInfo = Variant::getTypeInfo<glm::ivec3>()},
                        PropertySpec{.name = "position_world",
                                     .typeInfo = Variant::getTypeInfo<glm::vec4>()},
                        PropertySpec{.name = "normal",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                    },
                .outputSpecs =
                    {
                        PropertySpec{.name = "shade_ambient",
                                     .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                    },
                .snippet = R"(
                    void setGlobalLighting(
                        in vec3 giWorldStart, in ivec3 giGridSize,
                        in vec4 position_world, in vec3 normal,
                        out vec3 shade_ambient
                    ) {
                        shade_ambient = vec3(0.0);
                    }
                )",
                .functionName = "setGlobalLighting"}});

        p_fragmentMethod =
            dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
                .tasks = {p_shadeAmbient}, .friendlyName = "GlobalIllumination"});

        /*
        COMPUTE SHADING
        */

        p_computeMethod = dynasma::makeStandalone<Method<ShaderTask>>(
            Method<ShaderTask>::MethodParams{.tasks = {}, .friendlyName = "GlobalIllumination"});

        /*
        COMPOSING
        */

        p_composeMethod = dynasma::makeStandalone<Method<ComposeTask>>(
            Method<ComposeTask>::MethodParams{.tasks = {}, .friendlyName = "GlobalIllumination"});

        /*
        SETUP
        */
        setupFunctions.push_back([](ComponentRoot &root, Renderer &renderer, ScopedDict &dict) {
            MMETER_SCOPE_PROFILER("GI type specification");

            OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(renderer);

            rend.specifyGlType({
                .glMutableTypeName = "Transfer",
                .glConstTypeName = "Transfer",
                .glslDefinitionSnippet = GLSL_TRANSFER_DEF_SNIPPET,
                .memberTypeDependencies = {&rend.getGlTypeSpec("vec4")},
                .std140Size = 16,
                .std140Alignment = 16,
                .layoutIndexSize = 1,
            });
            rend.specifyGlType({
                .glMutableTypeName = "Source2FacesTransfer",
                .glConstTypeName = "Source2FacesTransfer",
                .glslDefinitionSnippet = GLSL_S2F_TRANSFER_DEF_SNIPPET,
                .memberTypeDependencies = {&rend.getGlTypeSpec("Transfer")},
                .std140Size = 16 * 6,
                .std140Alignment = 16,
                .layoutIndexSize = 1 * 6,
            });
            rend.specifyGlType({
                .glMutableTypeName = "NeighborTransfer",
                .glConstTypeName = "NeighborTransfer",
                .glslDefinitionSnippet = GLSL_NEIGHBOR_TRANSFER_DEF_SNIPPET,
                .memberTypeDependencies = {&rend.getGlTypeSpec("Source2FacesTransfer")},
                .std140Size = 16 * 6 * 6,
                .std140Alignment = 16,
                .layoutIndexSize = 1 * 6 * 6,
            });
            rend.specifyGlType({
                .glMutableTypeName = "ProbeDefinition",
                .glConstTypeName = "ProbeDefinition",
                .glslDefinitionSnippet = GLSL_PROBE_DEF_SNIPPET,
                .memberTypeDependencies = {&rend.getGlTypeSpec("vec4"),
                                           &rend.getGlTypeSpec("uint")},
                .std140Size = 40,
                .std140Alignment = 16,
                .layoutIndexSize = 4,
            });
            rend.specifyTypeConversion({
                .hostType = Variant::getTypeInfo<G_ProbeDefinition>(),
                .glTypeSpec = rend.getGlTypeSpec("ProbeDefinition"),
            });

            rend.specifyGlType({
                .glMutableTypeName = "ProbeState",
                .glConstTypeName = "ProbeState",
                .glslDefinitionSnippet = GLSL_PROBE_STATE_SNIPPET,
                .memberTypeDependencies = {&rend.getGlTypeSpec("vec4"),
                                           &rend.getGlTypeSpec("uint")},
                .std140Size = 16 * 6,
                .std140Alignment = 16,
                .layoutIndexSize = 6,
            });
            rend.specifyTypeConversion({
                .hostType = Variant::getTypeInfo<G_ProbeState>(),
                .glTypeSpec = rend.getGlTypeSpec("ProbeState"),
            });

            rend.specifyBufferTypeAndConversionAuto<ProbeBufferPtr>("ProbeBuffer");
            rend.specifyBufferTypeAndConversionAuto<ProbeStateBufferPtr>("ProbeStateBuffer");
        });
        setupFunctions.push_back([](ComponentRoot &root, Renderer &renderer, ScopedDict &dict) {
            MMETER_SCOPE_PROFILER("GI setup");

            // get the AABB of the scene
            const Scene &scene = *dict.get("scene").get<dynasma::FirmPtr<Scene>>();
            BoundingBox sceneAABB = scene.meshProps[0].p_mesh->getBoundingBox();
            for (const auto &mesh : scene.meshProps) {
                sceneAABB.merge(mesh.p_mesh->getBoundingBox());
            }

            // generate world
            std::vector<GI::H_ProbeDefinition> probes;
            glm::ivec3 gridSize;
            glm::vec3 worldStart;
            ProbeBufferPtr gpuProbes(root,
                                     (BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW));
            ProbeStateBufferPtr gpuProbeStates(root, (BufferUsageHint::HOST_INIT |
                                                      BufferUsageHint::GPU_COMPUTE |
                                                      BufferUsageHint::GPU_DRAW));
            ReflectionBufferPtr gpuReflectionTransfers(
                root, (BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW));
            LeavingPremulFactorBufferPtr gpuLeavingPremulFactors(
                root, (BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW));
            NeighborIndexBufferPtr gpuNeighborIndices(
                root, (BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW));
            NeighborTransferBufferPtr gpuNeighborTransfer(
                root, (BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW));

            generateProbeList(probes, gridSize, worldStart, sceneAABB.getCenter(),
                              sceneAABB.getExtent(), 200.0f);
            convertHost2GpuBuffers(probes, gpuProbes, gpuReflectionTransfers,
                                   gpuLeavingPremulFactors, gpuNeighborIndices,
                                   gpuNeighborTransfer);

            gpuProbeStates.resizeElements(probes.size());
            for (std::size_t i = 0; i < probes.size(); ++i) {
                for (std::size_t j = 0; j < 6; ++j) {
                    gpuProbeStates.getElement(i).illumination[j] = glm::vec4(0.0);
                }
            }

            // store properties
            dict.set("gpuProbes", gpuProbes);
            dict.set("gpuProbeStates", gpuProbeStates);
            dict.set("gpuReflectionTransfers", gpuReflectionTransfers);
            dict.set("gpuLeavingPremulFactors", gpuLeavingPremulFactors);
            dict.set("gpuNeighborIndices", gpuNeighborIndices);
            dict.set("gpuNeighborTransfer", gpuNeighborTransfer);
            dict.set("giWorldStart", worldStart);
            dict.set("giGridSize", gridSize);
        });
    }
};