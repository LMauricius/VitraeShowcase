#pragma once

#include "Probe.hpp"

namespace GI
{

extern const char *const GLSL_PROBE_GEN_SNIPPET;

void generateProbeList(std::vector<H_ProbeDefinition> &probes, glm::uvec3 &gridSize,
                       glm::vec3 &worldStart, glm::vec3 worldCenter, glm::vec3 worldSize,
                       float minProbeSize, bool cpuTransferGen);

void generateTransfers(std::vector<H_ProbeDefinition> &probes,
                       NeighborTransferBufferPtr gpuNeighborTransfers,
                       NeighborFilterBufferPtr gpuNeighborFilters);
} // namespace GI