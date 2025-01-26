#pragma once

#include "abstract.hpp"

#include "GI/Generation.hpp"
#include "GI/Probe.hpp"

#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/Compute.hpp"
#include "Vitrae/Pipelines/Compositing/DataRender.hpp"
#include "Vitrae/Pipelines/Compositing/FrameToTexture.hpp"
#include "Vitrae/Pipelines/Compositing/Function.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Shading/Constant.hpp"
#include "Vitrae/Pipelines/Shading/Header.hpp"
#include "Vitrae/Pipelines/Shading/Snippet.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"

#include "dynasma/standalone.hpp"

#include "MMeter.h"

#include <iostream>

namespace VitraeGI
{
using namespace Vitrae;

inline void setupGI(ComponentRoot &root, bool allowVisualization) : MethodCollection(root)
{
    String friendlyName = "GlobalIllumination";
    if (allowVisualization) {
        friendlyName += "_visualization";
    }

    /*
    VERTEX SHADING
    */

    p_vertexMethod =
        dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{.tasks = {}});

    /*
    GENERIC SHADING
    */

    auto p_giHeader = root.getComponent<ShaderHeaderKeeper>().new_asset(
        {ShaderHeader::StringParams{.inputTokenNames = {},
                                    .outputTokenNames = {"gi_utilities"},
                                    .snippet = GI::GLSL_PROBE_UTILITY_SNIPPET,
                                    .friendlyName = "giConstants"}});

    p_genericShaderMethod = dynasma::makeStandalone<Method<ShaderTask>>(
        Method<ShaderTask>::MethodParams{.tasks = {p_giHeader}, .friendlyName = friendlyName});

    /*
    FRAGMENT SHADING
    */

    auto p_shadeAmbient =
        root.getComponent<ShaderFunctionKeeper>().new_asset({ShaderFunction::StringParams{
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
                                 .typeInfo = Variant::getTypeInfo<glm::uvec3>()},
                    PropertySpec{.name = "position_world",
                                 .typeInfo = Variant::getTypeInfo<glm::vec4>()},
                    PropertySpec{.name = "normal", .typeInfo = Variant::getTypeInfo<glm::vec3>()},
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
                        in vec3 giWorldStart, in vec3 giWorldSize, in uvec3 giGridSize,
                        in vec4 position_world, in vec3 normal,
                        out vec3 shade_ambient
                    ) {
                        uvec3 gridPos = uvec3(floor(
                            (position_world.xyz - giWorldStart) / giWorldSize * vec3(giGridSize)
                        ));
                        gridPos = clamp(gridPos, uvec3(0), giGridSize - uvec3(1));

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
                    }
                    )",
            .functionName = "setGlobalLighting"}});

    p_fragmentMethod = dynasma::makeStandalone<Method<ShaderTask>>(
        Method<ShaderTask>::MethodParams{.tasks = {p_shadeAmbient}, .friendlyName = friendlyName});

    /*
    COMPUTE SHADING
    */

    auto
        p_giPropagate = root.getComponent<ShaderFunctionKeeper>()
                            .new_asset({ShaderFunction::
                                            StringParams{
                                                .inputSpecs =
                                                    {
                                                        PropertySpec{
                                                            .name = "gpuProbeStates_prev",
                                                            .typeInfo = Variant::getTypeInfo<
                                                                ProbeStateBufferPtr>()},
                                                        PropertySpec{.name = "gpuProbeStates",
                                                                     .typeInfo = Variant::getTypeInfo<ProbeStateBufferPtr>()},
                                                        PropertySpec{
                                                            .name = "gpuProbes",
                                                            .typeInfo =
                                                                Variant::getTypeInfo<
                                                                    ProbeBufferPtr>()},
                                                        PropertySpec{
                                                            .name = "gpuNeighborIndices",
                                                            .typeInfo = Variant::getTypeInfo<
                                                                NeighborIndexBufferPtr>()},
                                                        PropertySpec{
                                                            .name = "gpuReflectionTransfers",
                                                            .typeInfo = Variant::getTypeInfo<ReflectionBufferPtr>()},
                                                        PropertySpec{.name = "gpuNeighborTransfer",
                                                                     .typeInfo = Variant::
                                                                         getTypeInfo<
                                                                             NeighborTransferBufferPtr>()},
                                                        PropertySpec{
                                                            .name = "gpuNeighborFilters",
                                                            .typeInfo = Variant::getTypeInfo<
                                                                NeighborFilterBufferPtr>()},
                                                        PropertySpec{
                                                            .name = "gpuLeavingPremulFactors",
                                                            .typeInfo = Variant::getTypeInfo<
                                                                LeavingPremulFactorBufferPtr>()},
                                                        PropertySpec{
                                                            .name = "giWorldSize",
                                                            .typeInfo =
                                                                Variant::getTypeInfo<glm::vec3>()},
                                                        PropertySpec{.name = "giGridSize",
                                                                     .typeInfo = Variant::
                                                                         getTypeInfo<glm::uvec3>()},
                                                        PropertySpec{
                                                            .name = "camera_position",
                                                            .typeInfo =
                                                                Variant::getTypeInfo<glm::vec3>()},

                                                        PropertySpec{
                                                            .name = "gi_utilities",
                                                            .typeInfo = Variant::getTypeInfo<
                                                                void>()},
                                                        PropertySpec{
                                                            .name = "swapped_probes",
                                                            .typeInfo =
                                                                Variant::getTypeInfo<void>()},
                                                    },
                                                .outputSpecs =
                                                    {
                                                        PropertySpec{
                                                            .name = "updated_probes",
                                                            .typeInfo =
                                                                Variant::getTypeInfo<void>()},
                                                    },
                                                .snippet = R"(
void giPropagate(
    in vec3 giWorldSize, in uvec3 giGridSize, in vec3 camera_position
) {
    uint probeIndex = gl_GlobalInvocationID.x;
    uint faceIndex = gl_GlobalInvocationID.y;

    vec3 probeSize = buffer_gpuProbes.elements[probeIndex].size;
    vec3 probePos = buffer_gpuProbes.elements[probeIndex].position;
    uint neighborStartInd = buffer_gpuProbes.elements[probeIndex].neighborSpecBufStart;
    uint neighborCount = buffer_gpuProbes.elements[probeIndex].neighborSpecCount;

    // reflection
    buffer_gpuProbeStates.elements[probeIndex].illumination[faceIndex] = vec4(0.0);
    for (uint reflFaceIndex = 0; reflFaceIndex < 6; reflFaceIndex++) {
        buffer_gpuProbeStates.elements[probeIndex].illumination[faceIndex] += (
            buffer_gpuProbeStates_prev.elements[probeIndex].illumination[reflFaceIndex] *
            buffer_gpuReflectionTransfers.elements[probeIndex].face[reflFaceIndex]
        );
    }

    // if camera is inside probe, glow
    if (all(lessThan(abs(camera_position - probePos), probeSize * 0.5))) {
        buffer_gpuProbeStates.elements[probeIndex].illumination[faceIndex] += vec4(5.0);
    } else {
    }
    for (uint i = neighborStartInd; i < neighborStartInd + neighborCount; i++) {
        uint neighInd = buffer_gpuNeighborIndices.elements[i];
        for (uint neighDirInd = 0; neighDirInd < 6; neighDirInd++) {
            buffer_gpuProbeStates.elements[probeIndex].illumination[faceIndex] += (
                buffer_gpuProbeStates_prev.elements[neighInd].illumination[neighDirInd] *
                buffer_gpuNeighborFilters.elements[neighInd] *
                buffer_gpuNeighborTransfer.elements[neighInd].source[faceIndex].face[neighDirInd] *
                buffer_gpuLeavingPremulFactors.elements[probeIndex].face[faceIndex]
            );
        }
    }

    uvec3 gridPos = uvec3(
        probeIndex / giGridSize.y / giGridSize.z,
        (probeIndex / giGridSize.z) % giGridSize.y,
        probeIndex % giGridSize.z
    );
}
                )",
                                                .functionName = "giPropagate"}});

    auto p_generateTransfersShader =
        root.getComponent<ShaderFunctionKeeper>().new_asset({ShaderFunction::StringParams{
            .inputSpecs =
                {
                    PropertySpec{.name = "giGridSize",
                                 .typeInfo = Variant::getTypeInfo<glm::uvec3>()},
                    PropertySpec{.name = "gpuProbes",
                                 .typeInfo = Variant::getTypeInfo<ProbeBufferPtr>()},
                    PropertySpec{.name = "gpuReflectionTransfers",
                                 .typeInfo = Variant::getTypeInfo<ReflectionBufferPtr>()},
                    PropertySpec{.name = "gpuNeighborIndices",
                                 .typeInfo = Variant::getTypeInfo<NeighborIndexBufferPtr>()},
                    PropertySpec{.name = "gpuNeighborTransfer",
                                 .typeInfo = Variant::getTypeInfo<NeighborTransferBufferPtr>()},
                    PropertySpec{.name = "gpuNeighborFilters",
                                 .typeInfo = Variant::getTypeInfo<NeighborFilterBufferPtr>()},
                    PropertySpec{.name = "gpuLeavingPremulFactors",
                                 .typeInfo = Variant::getTypeInfo<LeavingPremulFactorBufferPtr>()},

                    PropertySpec{.name = "gi_utilities", .typeInfo = Variant::getTypeInfo<void>()},
                },
            .outputSpecs =
                {
                    PropertySpec{.name = "generated_probe_transfers",
                                 .typeInfo = Variant::getTypeInfo<void>()},
                },
            .snippet = String(GI::GLSL_PROBE_GEN_SNIPPET) + R"(
void giGenerateTransfers(in uvec3 giGridSize) {
    uint probeIndex = gl_GlobalInvocationID.x;
    uint myDirInd = gl_GlobalInvocationID.y;
    uvec3 gridPos = uvec3(
        probeIndex / giGridSize.y / giGridSize.z,
        (probeIndex / giGridSize.z) % giGridSize.y,
        probeIndex % giGridSize.z
    );

    uint neighborStartInd = buffer_gpuProbes.elements[probeIndex].neighborSpecBufStart;
    uint neighborCount = buffer_gpuProbes.elements[probeIndex].neighborSpecCount;

    float totalLeaving = 0.0;
    for (uint i = neighborStartInd; i < neighborStartInd + neighborCount; i++) {
        uint neighInd = buffer_gpuNeighborIndices.elements[i];
        for (uint neighDirInd = 0; neighDirInd < 6; neighDirInd++) {
            if (all(lessThan(gridPos, giGridSize-2)) &&
                all(greaterThan(gridPos, uvec3(1)))) {
                
                buffer_gpuNeighborFilters.elements[neighInd] = vec4(1.0);
            } else {
                buffer_gpuNeighborFilters.elements[neighInd] = vec4(0);
            }
            buffer_gpuNeighborTransfer.elements[neighInd].
                source[neighDirInd].face[myDirInd] =
                factorTo(neighInd, probeIndex, neighDirInd, myDirInd);
            totalLeaving +=
                factorTo(probeIndex, neighInd, myDirInd, neighDirInd);
        }
    }

    if (totalLeaving > 0.05) {
        buffer_gpuLeavingPremulFactors.elements[probeIndex].face[myDirInd] = 0.95 / totalLeaving;
    } else {
        buffer_gpuLeavingPremulFactors.elements[probeIndex].face[myDirInd] = 0.0;
    }

    // reflection
    buffer_gpuReflectionTransfers.elements[probeIndex].face[myDirInd] = vec4(
        0.0, 0.0, 0.0, 0.0
    );
}
                )",
            .functionName = "giGenerateTransfers"}});

    p_computeMethod = dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
        .tasks = {p_giPropagate, p_generateTransfersShader}, .friendlyName = friendlyName});

    /*
    COMPOSING
    */
    auto p_swapProbeBuffers = dynasma::makeStandalone<ComposeFunction>(ComposeFunction::SetupParams{
        .inputSpecs =
            {
                PropertySpec{.name = "gpuProbeStates",
                             .typeInfo = Variant::getTypeInfo<ProbeStateBufferPtr>()},
                PropertySpec{.name = "generated_probe_transfers",
                             .typeInfo = Variant::getTypeInfo<void>()},
            },
        .outputSpecs =
            {
                PropertySpec{.name = "gpuProbeStates_prev",
                             .typeInfo = Variant::getTypeInfo<ProbeStateBufferPtr>()},
                PropertySpec{.name = "gpuProbeStates",
                             .typeInfo = Variant::getTypeInfo<ProbeStateBufferPtr>()},
                PropertySpec{.name = "swapped_probes", .typeInfo = Variant::getTypeInfo<void>()},
            },
        .p_function =
            [&root](const RenderRunContext &context) {
                auto gpuProbeStates =
                    context.properties.get("gpuProbeStates").get<ProbeStateBufferPtr>();
                ProbeStateBufferPtr gpuProbeStates_prev;

                // allocate the new buffer if we don't have it yet
                if (auto pv_gpuProbeStates_prev = context.properties.getPtr("gpuProbeStates_prev");
                    pv_gpuProbeStates_prev == nullptr) {

                    gpuProbeStates_prev = ProbeStateBufferPtr(root,
                                                              BufferUsageHint::HOST_INIT |
                                                                  BufferUsageHint::GPU_COMPUTE |
                                                                  BufferUsageHint::GPU_DRAW,
                                                              gpuProbeStates.numElements());
                    for (uint i = 0; i < gpuProbeStates.numElements(); i++) {
                        for (uint j = 0; j < 6; j++) {
                            gpuProbeStates_prev.getElement(i).illumination[j] = glm::vec4(0.0f);
                        }
                    }

                    gpuProbeStates_prev.getRawBuffer()->synchronize();
                } else {
                    gpuProbeStates_prev = pv_gpuProbeStates_prev->get<ProbeStateBufferPtr>();
                }

                // swap buffers
                context.properties.set("gpuProbeStates_prev", gpuProbeStates);
                context.properties.set("gpuProbeStates", gpuProbeStates_prev);
            },
        .friendlyName = "Swap probe buffers",
    });

    auto p_generateProbeTransfers =
        root.getComponent<ComposeComputeKeeper>().new_asset({ComposeCompute::SetupParams{
            .root = root,
            .outputSpecs =
                {
                    PropertySpec{.name = "generated_probe_transfers",
                                 .typeInfo = Variant::getTypeInfo<void>()},
                },
            .computeSetup =
                {
                    .invocationCountX = {"gpuProbeCount"},
                    .invocationCountY = 6,
                    .groupSizeY = 6,
                },
            .executeCondition =
                [](RenderRunContext &context) {
                    if (context.properties.has("generated_probe_transfers")) {
                        return false;
                    } else {
                        context.properties.set("generated_probe_transfers", true);
                        return true;
                    }
                },
        }});

    auto p_updateProbes = root.getComponent<ComposeComputeKeeper>().new_asset(
        {ComposeCompute::SetupParams{.root = root,
                                     .outputSpecs =
                                         {
                                             PropertySpec{.name = "updated_probes",
                                                          .typeInfo = Variant::getTypeInfo<void>()},
                                         },
                                     .computeSetup = {
                                         .invocationCountX = {"gpuProbeCount"},
                                         .invocationCountY = 6,
                                         .groupSizeY = 6,
                                     }}});

    auto p_visualScene = dynasma::makeStandalone<Scene>(
        Scene::FileLoadParams{.root = root, .filepath = "media/dataPoint/dataPoint.obj"});
    auto p_visualMesh = p_visualScene->meshProps[0].p_mesh;

    auto p_dataRender =
        root.getComponent<ComposeDataRenderKeeper>().new_asset({ComposeDataRender::SetupParams{
            .root = root,
            .inputSpecs =
                {
                    PropertySpec{.name = "giSamples",
                                 .typeInfo = Variant::getTypeInfo<std::vector<Sample>>()},
                },
            .displayInputPropertyName = "display_cleared",
            .displayOutputPropertyName = "visualized_samples",
            .p_dataPointVisual = p_visualMesh,
            .dataGenerator =
                [](const RenderRunContext &context, ComposeDataRender::RenderCallback callback) {
                    auto &samples = context.properties.get("giSamples").get<std::vector<Sample>>();

                    for (auto &sample : samples) {
                        SimpleTransformation trans;
                        trans.position = sample.position;
                        // trans.rotation = glm::quatLookAt(sample.normal, glm::vec3(0, 1, 0));
                        trans.rotation = glm::quat(glm::vec3(0, 0, 1), sample.normal);
                        trans.scaling = {1.0f, 1.0f, 1.0f};

                        callback(trans.getModelMatrix());
                    }
                },
            .vertexPositionOutputPropertyName = "position_view",
        }});

    p_composeMethod =
        dynasma::makeStandalone<Method<ComposeTask>>(Method<ComposeTask>::MethodParams{
            .tasks = {p_swapProbeBuffers, p_generateProbeTransfers, p_updateProbes, p_dataRender},
            .friendlyName = friendlyName});

    /*
    SETUP
    */
    setupFunctions.push_back([](ComponentRoot &root, Renderer &renderer, ScopedDict &dict) {
        MMETER_SCOPE_PROFILER("GI type specification");

        OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(renderer);

        rend.specifyGlType({
            .glMutableTypeName = "Reflection",
            .glConstTypeName = "Reflection",
            .glslDefinitionSnippet = GLSL_REFLECTION_DEF_SNIPPET,
            .memberTypeDependencies = {&rend.getGlTypeSpec("vec4")},
            .std140Size = 16 * 6,
            .std140Alignment = 16,
            .layoutIndexSize = 1 * 6,
        });
        rend.specifyTypeConversion({
            .hostType = Variant::getTypeInfo<Reflection>(),
            .glTypeSpec = rend.getGlTypeSpec("Reflection"),
        });

        rend.specifyGlType({
            .glMutableTypeName = "FaceTransfer",
            .glConstTypeName = "FaceTransfer",
            .glslDefinitionSnippet = GLSL_FACE_TRANSFER_DEF_SNIPPET,
            .memberTypeDependencies = {&rend.getGlTypeSpec("float")},
            .std140Size = 4 * 6,
            .std140Alignment = 4,
            .layoutIndexSize = 1 * 6,
        });
        rend.specifyTypeConversion({
            .hostType = Variant::getTypeInfo<FaceTransfer>(),
            .glTypeSpec = rend.getGlTypeSpec("FaceTransfer"),
        });

        rend.specifyGlType({
            .glMutableTypeName = "NeighborTransfer",
            .glConstTypeName = "NeighborTransfer",
            .glslDefinitionSnippet = GLSL_NEIGHBOR_TRANSFER_DEF_SNIPPET,
            .memberTypeDependencies = {&rend.getGlTypeSpec("FaceTransfer")},
            .std140Size = 4 * 6 * 6,
            .std140Alignment = 4,
            .layoutIndexSize = 1 * 6 * 6,
        });
        rend.specifyTypeConversion({
            .hostType = Variant::getTypeInfo<NeighborTransfer>(),
            .glTypeSpec = rend.getGlTypeSpec("NeighborTransfer"),
        });

        rend.specifyGlType({
            .glMutableTypeName = "ProbeDefinition",
            .glConstTypeName = "ProbeDefinition",
            .glslDefinitionSnippet = GLSL_PROBE_DEF_SNIPPET,
            .memberTypeDependencies = {&rend.getGlTypeSpec("vec4"), &rend.getGlTypeSpec("uint")},
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
            .memberTypeDependencies = {&rend.getGlTypeSpec("vec4")},
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
        rend.specifyBufferTypeAndConversionAuto<ReflectionBufferPtr>("ReflectionBuffer");
        rend.specifyBufferTypeAndConversionAuto<NeighborIndexBufferPtr>("NeighborIndexBuffer");
        rend.specifyBufferTypeAndConversionAuto<NeighborTransferBufferPtr>(
            "NeighborTransferBuffer");
        rend.specifyBufferTypeAndConversionAuto<NeighborFilterBufferPtr>("NeighborFilterBuffer");
        rend.specifyBufferTypeAndConversionAuto<LeavingPremulFactorBufferPtr>(
            "LeavingPremulFactorBuffer");
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
        glm::uvec3 gridSize;
        glm::vec3 worldStart;
        ProbeBufferPtr gpuProbes(root, (BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW),
                                 "gpuProbes");
        ProbeStateBufferPtr gpuProbeStates(
            root,
            (BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_COMPUTE | BufferUsageHint::GPU_DRAW),
            "gpuProbeStates");
        ReflectionBufferPtr gpuReflectionTransfers(
            root, (BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW),
            "gpuReflectionTransfers");
        LeavingPremulFactorBufferPtr gpuLeavingPremulFactors(
            root, (BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW),
            "gpuLeavingPremulFactors");
        NeighborIndexBufferPtr gpuNeighborIndices(
            root, (BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW), "gpuNeighborIndices");
        NeighborTransferBufferPtr gpuNeighborTransfer(
            root, (BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW), "gpuNeighborTransfer");
        NeighborFilterBufferPtr gpuNeighborFilters(
            root, (BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW), "gpuNeighborFilters");

        SamplingScene smpScene;
        std::size_t numNullMeshes = 0, numNullTris = 0;
        prepareScene(scene, smpScene, numNullMeshes, numNullTris);
        std::vector<Sample> samples;
        sampleScene(smpScene, 30000, samples);
        generateProbeList(std::span<const Sample>(samples), probes, gridSize, worldStart,
                          sceneAABB.getCenter(), sceneAABB.getExtent(), 1.5f, false);
        convertHost2GpuBuffers(probes, gpuProbes, gpuReflectionTransfers, gpuLeavingPremulFactors,
                               gpuNeighborIndices, gpuNeighborTransfer, gpuNeighborFilters);
        // generateTransfers(probes, gpuNeighborTransfer, gpuNeighborFilters);

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
        gpuNeighborFilters.getRawBuffer()->synchronize();

        // store properties
        dict.set("giSamples", samples);
        dict.set("gpuProbes", gpuProbes);
        dict.set("gpuProbeStates", gpuProbeStates);
        dict.set("gpuReflectionTransfers", gpuReflectionTransfers);
        dict.set("gpuLeavingPremulFactors", gpuLeavingPremulFactors);
        dict.set("gpuNeighborIndices", gpuNeighborIndices);
        dict.set("gpuNeighborTransfer", gpuNeighborTransfer);
        dict.set("gpuNeighborFilters", gpuNeighborFilters);
        dict.set("giWorldStart", worldStart);
        dict.set("giWorldSize", sceneAABB.getExtent());
        dict.set("giGridSize", gridSize);
        dict.set("gpuProbeCount", (std::uint32_t)gpuProbes.numElements());

        // Print stats
        root.getInfoStream() << "=== GI STATISTICS ===" << std::endl;
        root.getInfoStream() << "Null weight meshes: " << numNullMeshes << std::endl;
        root.getInfoStream() << "Null weight triangles: " << numNullTris << std::endl;
        root.getInfoStream() << "Probe count: " << probes.size() << std::endl;
        root.getInfoStream() << "gpuProbes size: " << gpuProbes.byteSize() << std::endl;
        root.getInfoStream() << "gpuProbeStates size: " << gpuProbeStates.byteSize() << std::endl;
        root.getInfoStream() << "gpuReflectionTransfers size: " << gpuReflectionTransfers.byteSize()
                             << std::endl;
        root.getInfoStream() << "gpuLeavingPremulFactors size: "
                             << gpuLeavingPremulFactors.byteSize() << std::endl;
        root.getInfoStream() << "gpuNeighborIndices size: " << gpuNeighborIndices.byteSize()
                             << std::endl;
        root.getInfoStream() << "gpuNeighborTransfer size: " << gpuNeighborTransfer.byteSize()
                             << std::endl;
        root.getInfoStream() << "gpuNeighborFilters size: " << gpuNeighborFilters.byteSize()
                             << std::endl;
        root.getInfoStream() << "Total GPU size: "
                             << (gpuProbes.byteSize() + gpuProbeStates.byteSize() +
                                 gpuReflectionTransfers.byteSize() +
                                 gpuLeavingPremulFactors.byteSize() +
                                 gpuNeighborIndices.byteSize() + gpuNeighborTransfer.byteSize() +
                                 gpuNeighborFilters.byteSize()) /
                                    1000000.0f
                             << " MB" << std::endl;
        root.getInfoStream() << "=== /GI STATISTICS ===" << std::endl;
    });

    if (allowVisualization) {
        desiredOutputs = PropertyList{PropertySpec{
            .name = "visualized_samples", .typeInfo = StandardCompositorOutputTypes::OUTPUT_TYPE}};
    }
}
}; // namespace VitraeGI