#pragma once

#include "../shadingModes.hpp"
#include "abstract.hpp"

#include "GI/Generation.hpp"
#include "GI/Probe.hpp"

#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/Compute.hpp"
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
            root.getComponent<ShaderFunctionKeeper>().new_asset(
                {ShaderFunction::StringParams{
                    .inputSpecs =
                        {
                            PropertySpec{.name = "gpuProbes",
                                         .typeInfo = Variant::getTypeInfo<ProbeBufferPtr>()},
                            PropertySpec{.name = "gpuProbeStates",
                                         .typeInfo = Variant::getTypeInfo<ProbeStateBufferPtr>()},
                            PropertySpec{.name = "giWorldStart",
                                         .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                            PropertySpec{.name = "giWorldSize",
                                         .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                            PropertySpec{.name = "giGridSize",
                                         .typeInfo = Variant::getTypeInfo<glm::ivec3>()},
                            PropertySpec{.name = "position_world",
                                         .typeInfo = Variant::getTypeInfo<glm::vec4>()},
                            PropertySpec{.name = "normal",
                                         .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                            PropertySpec{.name = "updated_probes",
                                         .typeInfo = Variant::getTypeInfo<void>()},
                        },
                    .outputSpecs =
                        {
                            PropertySpec{.name = "shade_ambient",
                                         .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                        },
                    .snippet = R"(
                    void setGlobalLighting(
                        in vec3 giWorldStart, in vec3 giWorldSize, in ivec3 giGridSize,
                        in vec4 position_world, in vec3 normal,
                        out vec3 shade_ambient
                    ) {
                        ivec3 gridPos = ivec3(floor(
                            (position_world.xyz - giWorldStart) / giWorldSize * vec3(giGridSize)
                        ));
                        gridPos = clamp(gridPos, ivec3(0), giGridSize - ivec3(1));

                        uint ind = gridPos.x * giGridSize.y * giGridSize.z + gridPos.y * giGridSize.z + gridPos.z;
                        vec3 probeSize = buffer_gpuProbes.elements[ind].size.xyz;//(giWorldSize / vec3(giGridSize));
                        vec3 probePos = buffer_gpuProbes.elements[ind].position.xyz;//giWorldStart + (vec3(gridPos) + 0.5) * probeSize;

                        bvec3 normalIsNeg = lessThan(normal, vec3(0.0));
                        vec3 absNormal = abs(normal);

                        shade_ambient = 
                                buffer_gpuProbeStates.elements[ind].illumination[
                                    0 + int(normalIsNeg.x)
                                ].rgb * absNormal.x +
                                buffer_gpuProbeStates.elements[ind].illumination[
                                    2 + int(normalIsNeg.y)
                                ].rgb * absNormal.y +
                                buffer_gpuProbeStates.elements[ind].illumination[
                                    4 + int(normalIsNeg.z)
                                ].rgb * absNormal.z;
                        //vec3 offset = abs(position_world.xyz - probePos) / probeSize * 2.0;
                        //vec3 darkening = min(vec3(1.0) - (offset - 0.9) / 0.1, vec3(1.0));
                        //float darkeningS = min(darkening.x, min(darkening.y, darkening.z));
                        //shade_ambient = vec3(gridPos) / vec3(giGridSize) * darkeningS; // debug
                    }
                )",
                    .functionName = "setGlobalLighting"}});

        p_fragmentMethod =
            dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
                .tasks = {p_shadeAmbient}, .friendlyName = "GlobalIllumination"});

        /*
        COMPUTE SHADING
        */

        auto p_giPropagate =
            root.getComponent<ShaderFunctionKeeper>().new_asset(
                {ShaderFunction::StringParams{
                    .inputSpecs =
                        {
                            PropertySpec{.name = "gpuProbeStates_prev",
                                         .typeInfo = Variant::getTypeInfo<ProbeStateBufferPtr>()},
                            PropertySpec{.name = "gpuProbeStates",
                                         .typeInfo = Variant::getTypeInfo<ProbeStateBufferPtr>()},
                            PropertySpec{.name = "gpuProbes",
                                         .typeInfo = Variant::getTypeInfo<ProbeBufferPtr>()},
                            PropertySpec{.name = "giWorldSize",
                                         .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                            PropertySpec{.name = "camera_position",
                                         .typeInfo = Variant::getTypeInfo<glm::vec3>()},
                            PropertySpec{.name = "swapped_probes",
                                         .typeInfo = Variant::getTypeInfo<void>()},
                        },
                    .outputSpecs =
                        {
                            PropertySpec{.name = "gpuProbeStates",
                                         .typeInfo = Variant::getTypeInfo<ProbeStateBufferPtr>()},
                            PropertySpec{.name = "updated_probes",
                                         .typeInfo = Variant::getTypeInfo<void>()},
                        },
                    .snippet = R"(
                    void giPropagate(
                        in vec3 giWorldSize, in vec3 camera_position
                    ) {
                        uint probeIndex = gl_GlobalInvocationID.x;
                        uint faceIndex = gl_GlobalInvocationID.y;

                        vec3 probeSize = buffer_gpuProbes.elements[probeIndex].size;
                        vec3 probePos = buffer_gpuProbes.elements[probeIndex].position;

                        float worldScale = max(giWorldSize.x, max(giWorldSize.y, giWorldSize.z));
                        float scaledDist = distance(probePos, camera_position) / worldScale;

                        buffer_gpuProbeStates.elements[probeIndex].illumination[faceIndex] = vec4(min(
                            vec3(1.0),
                            vec3(5.0) / scaledDist / 100
                        ), 1.0);
                    }
                )",
                    .functionName = "giPropagate"}});

        p_computeMethod =
            dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
                .tasks = {p_giPropagate}, .friendlyName = "GlobalIllumination"});

        /*
        COMPOSING
        */
        auto p_swapProbeBuffers =
            dynasma::makeStandalone<ComposeFunction>(ComposeFunction::SetupParams{
                .inputSpecs =
                    {
                        PropertySpec{.name = "gpuProbeStates",
                                     .typeInfo = Variant::getTypeInfo<ProbeStateBufferPtr>()},
                    },
                .outputSpecs =
                    {
                        PropertySpec{.name = "gpuProbeStates_prev",
                                     .typeInfo = Variant::getTypeInfo<ProbeStateBufferPtr>()},
                        PropertySpec{.name = "gpuProbeStates",
                                     .typeInfo = Variant::getTypeInfo<ProbeStateBufferPtr>()},
                        PropertySpec{.name = "swapped_probes",
                                     .typeInfo = Variant::getTypeInfo<void>()},
                    },
                .p_function =
                    [&root](const RenderRunContext &context) {
                        auto gpuProbeStates =
                            context.properties.get("gpuProbeStates").get<ProbeStateBufferPtr>();
                        ProbeStateBufferPtr gpuProbeStates_prev;

                        // allocate the new buffer if we don't have it yet
                        if (auto pv_gpuProbeStates_prev =
                                context.properties.getPtr("gpuProbeStates_prev");
                            pv_gpuProbeStates_prev == nullptr) {
                            gpuProbeStates_prev = ProbeStateBufferPtr(
                                root, BufferUsageHint::GPU_COMPUTE | BufferUsageHint::GPU_DRAW,
                                gpuProbeStates.numElements());
                        } else {
                            gpuProbeStates_prev =
                                pv_gpuProbeStates_prev->get<ProbeStateBufferPtr>();
                        }

                        // swap buffers
                        context.properties.set("gpuProbeStates_prev", gpuProbeStates);
                        context.properties.set("gpuProbeStates", gpuProbeStates_prev);
                    },
                .friendlyName = "Swap probe buffers",
            });

        auto p_updateProbes =
            root.getComponent<ComposeComputeKeeper>().new_asset({ComposeCompute::SetupParams{
                .root = root,
                .outputSpecs =
                    {
                        PropertySpec{.name = "gpuProbeStates",
                                     .typeInfo = Variant::getTypeInfo<ProbeStateBufferPtr>()},
                        PropertySpec{.name = "updated_probes",
                                     .typeInfo = Variant::getTypeInfo<void>()},
                    },
                .computeSetup = {
                    .invocationCountX = {"gpuProbeCount"},
                    .invocationCountY = 6,
                    .groupSizeY = 6,
                }}});

        p_composeMethod = dynasma::makeStandalone<Method<ComposeTask>>(
            Method<ComposeTask>::MethodParams{.tasks = {p_swapProbeBuffers, p_updateProbes},
                                              .friendlyName = "GlobalIllumination"});

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
                .std140Size = 32,
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
            BoundingBox sceneAABB = transformed(scene.meshProps[0].transform.getModelMatrix(),
                                                scene.meshProps[0].p_mesh->getBoundingBox());
            for (const auto &meshProp : scene.meshProps) {
                sceneAABB.merge(transformed(meshProp.transform.getModelMatrix(),
                                            meshProp.p_mesh->getBoundingBox()));
            }
            sceneAABB.expand(1.1); // make it a bit bigger than the model

            // generate world
            std::vector<GI::H_ProbeDefinition> probes;
            glm::ivec3 gridSize;
            glm::vec3 worldStart;
            ProbeBufferPtr gpuProbes(root, (BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW),
                                     "gpuProbes");
            ProbeStateBufferPtr gpuProbeStates(root,
                                               (BufferUsageHint::HOST_INIT |
                                                BufferUsageHint::GPU_COMPUTE |
                                                BufferUsageHint::GPU_DRAW),
                                               "gpuProbeStates");
            ReflectionBufferPtr gpuReflectionTransfers(
                root, (BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW),
                "gpuReflectionTransfers");
            LeavingPremulFactorBufferPtr gpuLeavingPremulFactors(
                root, (BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW),
                "gpuLeavingPremulFactors");
            NeighborIndexBufferPtr gpuNeighborIndices(
                root, (BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW),
                "gpuNeighborIndices");
            NeighborTransferBufferPtr gpuNeighborTransfer(
                root, (BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW),
                "gpuNeighborTransfer");

            generateProbeList(probes, gridSize, worldStart, sceneAABB.getCenter(),
                              sceneAABB.getExtent(), 5.0f);
            convertHost2GpuBuffers(probes, gpuProbes, gpuReflectionTransfers,
                                   gpuLeavingPremulFactors, gpuNeighborIndices,
                                   gpuNeighborTransfer);

            gpuProbeStates.resizeElements(probes.size());
            for (std::size_t i = 0; i < probes.size(); ++i) {
                for (std::size_t j = 0; j < 6; ++j) {
                    gpuProbeStates.getElement(i).illumination[j] = glm::vec4(0.0);
                }
            }

            gpuProbes.getRawBuffer()->synchronize();
            gpuProbeStates.getRawBuffer()->synchronize();
            gpuReflectionTransfers.getRawBuffer()->synchronize();
            gpuLeavingPremulFactors.getRawBuffer()->synchronize();
            gpuNeighborIndices.getRawBuffer()->synchronize();
            gpuNeighborTransfer.getRawBuffer()->synchronize();

            // store properties
            dict.set("gpuProbes", gpuProbes);
            dict.set("gpuProbeStates", gpuProbeStates);
            dict.set("gpuReflectionTransfers", gpuReflectionTransfers);
            dict.set("gpuLeavingPremulFactors", gpuLeavingPremulFactors);
            dict.set("gpuNeighborIndices", gpuNeighborIndices);
            dict.set("gpuNeighborTransfer", gpuNeighborTransfer);
            dict.set("giWorldStart", worldStart);
            dict.set("giWorldSize", sceneAABB.getExtent());
            dict.set("giGridSize", gridSize);
            dict.set("gpuProbeCount", gpuProbes.numElements());
        });
    }
};