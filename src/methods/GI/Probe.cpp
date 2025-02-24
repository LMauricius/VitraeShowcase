#include "Probe.hpp"

#include "MMeter.h"

void GI::convertHost2GpuBuffers(std::span<const H_ProbeDefinition> hostProbes,
                                ProbeBufferPtr gpuProbes,
                                ReflectionBufferPtr gpuReflectionTransfers,
                                LeavingPremulFactorBufferPtr gpuLeavingPremulFactors,
                                NeighborIndexBufferPtr gpuNeighborIndices,
                                NeighborTransferBufferPtr gpuNeighborTransfers,
                                NeighborFilterBufferPtr gpuNeighborFilters)
{
    MMETER_FUNC_PROFILER;

    gpuProbes.resizeElements(hostProbes.size());
    gpuReflectionTransfers.resizeElements(hostProbes.size());
    gpuLeavingPremulFactors.resizeElements(hostProbes.size());

    std::size_t numNeighborSpecs = 0;

    for (std::size_t i = 0; i < hostProbes.size(); i++) {
        auto &hostProbe = hostProbes[i];
        auto &gpuProbe = gpuProbes.getMutableElement(i);
        auto &gpuReflectionTransfer = gpuReflectionTransfers.getMutableElement(i);
        auto &gpuLeavingPremulFactor = gpuLeavingPremulFactors.getMutableElement(i);

        gpuProbe.position = hostProbe.position;
        gpuProbe.size = hostProbe.size;
        gpuProbe.neighborSpecBufStart = numNeighborSpecs;
        gpuProbe.neighborSpecCount = hostProbe.neighborIndices.size();

        for (std::size_t d = 0; d < 6; d++) {
            gpuReflectionTransfer.face[d] = hostProbe.reflection.face[d];
            gpuLeavingPremulFactor.face[d] = hostProbe.leavingPremulFactor.face[d];
        }

        numNeighborSpecs += hostProbe.neighborIndices.size();
    }

    gpuNeighborIndices.resizeElements(numNeighborSpecs);
    gpuNeighborTransfers.resizeElements(numNeighborSpecs);
    gpuNeighborFilters.resizeElements(numNeighborSpecs);

    for (std::size_t i = 0; i < hostProbes.size(); i++) {
        auto &hostProbe = hostProbes[i];
        auto &gpuProbe = gpuProbes.getElement(i);

        for (std::size_t j = 0; j < gpuProbe.neighborSpecCount; j++) {
            gpuNeighborIndices.getMutableElement(gpuProbe.neighborSpecBufStart + j) =
                hostProbe.neighborIndices[j];

            auto &gpuNeighborTransfer =
                gpuNeighborTransfers.getElement(gpuProbe.neighborSpecBufStart + j);
        }
    }
}