#include "Probe.hpp"

#include "MMeter.h"

void GI::convertHost2GpuBuffers(std::span<const H_ProbeDefinition> hostProbes,
                                ProbeBufferPtr gpuProbes,
                                ReflectionBufferPtr gpuReflectionTransfers,
                                LeavingPremulFactorBufferPtr gpuLeavingPremulFactors,
                                NeighborIndexBufferPtr gpuNeighborIndices,
                                NeighborTransferBufferPtr gpuNeighborTransfers)
{
    MMETER_FUNC_PROFILER;

    gpuProbes.resizeElements(hostProbes.size());
    gpuReflectionTransfers.resizeElements(hostProbes.size());
    gpuLeavingPremulFactors.resizeElements(hostProbes.size());

    std::size_t numNeighborSpecs = 0;

    for (std::size_t i = 0; i < hostProbes.size(); i++) {
        auto &hostProbe = hostProbes[i];
        auto &gpuProbe = gpuProbes.getElement(i);
        auto &gpuReflectionTransfer = gpuReflectionTransfers.getElement(i);
        auto &gpuLeavingPremulFactor = gpuLeavingPremulFactors.getElement(i);

        gpuProbe.position = hostProbe.position;
        gpuProbe.size = hostProbe.size;
        gpuProbe.neighborSpecBufStart = numNeighborSpecs;
        gpuProbe.neighborSpecCount = hostProbe.neighborSpecs.size();

        for (std::size_t d = 0; d < 6; d++) {
            gpuReflectionTransfer.face[d] = glm::vec4(hostProbe.reflectionTransfer.face[d], 0.0);
            gpuLeavingPremulFactor[d] = hostProbe.leavingPremulFactor[d];
        }

        numNeighborSpecs += hostProbe.neighborSpecs.size();
    }

    gpuNeighborIndices.resizeElements(numNeighborSpecs);
    gpuNeighborTransfers.resizeElements(numNeighborSpecs);

    for (std::size_t i = 0; i < hostProbes.size(); i++) {
        auto &hostProbe = hostProbes[i];
        auto &gpuProbe = gpuProbes.getElement(i);

        for (std::size_t j = 0; j < gpuProbe.neighborSpecCount; j++) {
            auto &hostNeighborSpec = hostProbe.neighborSpecs[j];

            gpuNeighborIndices.getElement(gpuProbe.neighborSpecBufStart + j) =
                hostNeighborSpec.index;

            auto &gpuNeighborTransfer =
                gpuNeighborTransfers.getElement(gpuProbe.neighborSpecBufStart + j);

            for (std::size_t d1 = 0; d1 < 6; d1++) {
                for (std::size_t d2 = 0; d2 < 6; d2++) {
                    gpuNeighborTransfer.source[d1].face[d2] =
                        glm::vec4(hostNeighborSpec.transfer.source[d1].face[d2], 0.0);
                }
            }
        }
    }
}