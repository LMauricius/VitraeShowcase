#include "Generation.hpp"

#include "MMeter.h"

namespace GI
{

namespace
{

constexpr float PI2 = 3.14159265 * 2.0;

// 2*pi / light count over a round arc
constexpr float LIGHT_ARC_COVERAGE = PI2 / 4;

float factorTo(const H_ProbeDefinition &srcProbe, const H_ProbeDefinition &dstProbe,
               int srcDirIndex, int dstDirIndex)
{
    // note: we don't need the exact angular surface for the wall, or exact scales here.
    // Since the total leaving amounts get normalized,
    // the shared scalings (such as pi^2 constants) get nullified.

    const glm::vec3 &srcCenter = srcProbe.position;
    glm::vec3 wallCenter = dstProbe.position + DIRECTIONS[dstDirIndex] * dstProbe.size / 2.0f;
    float wallSize = 1.0;
    {
        glm::vec3 wallDiag = (glm::vec3(1.0) - glm::abs(DIRECTIONS[dstDirIndex])) * dstProbe.size;
        glm::vec3 wallDiagNon0 = wallDiag + glm::abs(DIRECTIONS[dstDirIndex]);
        wallSize = wallDiagNon0.x * wallDiagNon0.y * wallDiagNon0.z;
    }

    glm::vec3 src2wallOffset = wallCenter - srcCenter;
    float src2wallDist = glm::length(src2wallOffset);
    glm::vec3 src2wallDir = src2wallOffset / src2wallDist;

    float wallDot = glm::dot(src2wallDir, DIRECTIONS[dstDirIndex]);
    float lightDot = glm::dot(src2wallDir, DIRECTIONS[srcDirIndex]);
    float wallAngularSurface = wallSize / (src2wallDist * src2wallDist) * wallDot;

    if (wallAngularSurface <= 0.0f) {
        return 0.0f;
    }

    float wallArcCoverage = std::sqrt(wallSize) / (PI2 * src2wallDist) * wallDot;
    float wallArcOffset = std::abs(std::acos(lightDot));

    float wallArcStart = wallArcOffset - wallArcCoverage / 2.0f;
    float wallArcEnd = wallArcOffset + wallArcCoverage / 2.0f;
    float visibleAmount =
        std::max(std::min(LIGHT_ARC_COVERAGE / 2.0f, wallArcEnd), 0.0f) / wallArcCoverage;

    return visibleAmount * wallAngularSurface;
}

} // namespace

const char *const GLSL_PROBE_GEN_SNIPPET = R"(
const float PI2 = 3.14159265 * 2.0;
const float LIGHT_ARC_COVERAGE = PI2 / 4;

float factorTo(uint srcProbeindex, uint dstProbeindex, uint srcDirIndex, uint dstDirIndex)
{
    // note: we don't need the exact angular surface for the wall, or exact scales here.
    // Since the total leaving amounts get normalized,
    // the shared scalings (such as pi^2 constants) get nullified.

    vec3 srcCenter = buffer_gpuProbes.elements[srcProbeindex].position;
    vec3 wallCenter = buffer_gpuProbes.elements[dstProbeindex].position + DIRECTIONS[dstDirIndex] * buffer_gpuProbes.elements[dstProbeindex].size / 2.0f;
    float wallSize = 1.0;
    {
        vec3 wallDiag = (vec3(1.0) - abs(DIRECTIONS[dstDirIndex])) * buffer_gpuProbes.elements[dstProbeindex].size;
        vec3 wallDiagNon0 = wallDiag + abs(DIRECTIONS[dstDirIndex]);
        wallSize = wallDiagNon0.x * wallDiagNon0.y * wallDiagNon0.z;
    }

    vec3 src2wallOffset = wallCenter - srcCenter;
    float src2wallDist = length(src2wallOffset);
    vec3 src2wallDir = src2wallOffset / src2wallDist;

    float wallDot = dot(src2wallDir, DIRECTIONS[dstDirIndex]);
    float lightDot = dot(src2wallDir, DIRECTIONS[srcDirIndex]);
    float wallAngularSurface = wallSize / (src2wallDist * src2wallDist) * wallDot;

    if (wallAngularSurface <= 0.0f) {
        return 0.0f;
    }

    float wallArcCoverage = sqrt(wallSize) / (PI2 * src2wallDist) * wallDot;
    float wallArcOffset = abs(acos(lightDot));

    float wallArcStart = wallArcOffset - wallArcCoverage / 2.0f;
    float wallArcEnd = wallArcOffset + wallArcCoverage / 2.0f;
    float visibleAmount =
        max(min(LIGHT_ARC_COVERAGE / 2.0f, wallArcEnd), 0.0f) / wallArcCoverage;

    return visibleAmount * wallAngularSurface;
}
)";

void generateProbeList(std::vector<H_ProbeDefinition> &probes, glm::ivec3 &gridSize,
                       glm::vec3 &worldStart, glm::vec3 worldCenter, glm::vec3 worldSize,
                       float minProbeSize, bool cpuTransferGen)
{
    MMETER_FUNC_PROFILER;

    gridSize = glm::ivec3(worldSize / minProbeSize);
    worldStart = worldCenter - worldSize / 2.0f;

    probes.resize(gridSize.x * gridSize.y * gridSize.z);

    auto getIndex = [gridSize](glm::ivec3 ind) -> std::uint32_t {
        return ind.x * gridSize.y * gridSize.z + ind.y * gridSize.z + ind.z;
    };
    auto getProbe = [&](glm::ivec3 ind) -> auto & { return probes[getIndex(ind)]; };

    // edge probes' edges are put at world edges
    glm::vec3 probeSize = worldSize / glm::vec3(gridSize);
    glm::ivec3 minIndex = glm::ivec3(0, 0, 0);
    glm::ivec3 maxIndex = gridSize - 1;
    glm::vec3 ind2OffsetConversion = worldSize / glm::vec3(gridSize);

    {
        MMETER_SCOPE_PROFILER("Probe gen");

        for (int x = 0; x < gridSize.x; x++) {
            for (int y = 0; y < gridSize.y; y++) {
                for (int z = 0; z < gridSize.z; z++) {
                    auto &probe = getProbe({x, y, z});

                    probe.position =
                        worldStart + (glm::vec3(x, y, z) + 0.5f) * ind2OffsetConversion;
                    probe.size = probeSize;

                    if (x == 0 || x == maxIndex.x || y == 0 || y == maxIndex.y || z == 0 ||
                        z == maxIndex.z) {
                        // skip neighbor propagation for bordering probes
                    } else {

                        /*for (int neighx : {-1, 0, 1}) {
                            for (int neighy : {-1, 0, 1}) {
                                for (int neighz : {-1, 0, 1}) {
                                    if ((neighx == 0 && neighy == 0 && neighz == 0) || x + neighx <
                        0 || x + neighx >= maxIndex.x || y + neighy < 0 || y + neighy >= maxIndex.y
                        || z + neighz < 0 || z + neighz >= maxIndex.z) { continue;
                                    }
                                    probe.neighborIndices.push_back(
                                        getIndex({x + neighx, y + neighy, z + neighz}));
                                }
                            }
                        }*/

                        probe.neighborIndices.push_back(getIndex({x - 1, y, z}));
                        probe.neighborIndices.push_back(getIndex({x + 1, y, z}));
                        probe.neighborIndices.push_back(getIndex({x, y - 1, z}));
                        probe.neighborIndices.push_back(getIndex({x, y + 1, z}));
                        probe.neighborIndices.push_back(getIndex({x, y, z - 1}));
                        probe.neighborIndices.push_back(getIndex({x, y, z + 1}));
                    }
                }
            }
        }
    }

    if (cpuTransferGen) {}
}

void generateTransfers(std::vector<H_ProbeDefinition> &probes,
                       NeighborTransferBufferPtr gpuNeighborTransfers,
                       NeighborFilterBufferPtr gpuNeighborFilters)
{
    MMETER_SCOPE_PROFILER("Transfer gen");

    for (auto &probe : probes) {
        for (int myDirInd = 0; myDirInd < 6; myDirInd++) {
            float totalLeaving = 0.0;
            for (int neighDirInd = 0; neighDirInd < 6; neighDirInd++) {
                for (auto neighIndex : probe.neighborIndices) {

                    auto &neighTrans = gpuNeighborTransfers.getElement(neighIndex);
                    auto &neighFilter = gpuNeighborFilters.getElement(neighIndex);

                    neighTrans.source[neighDirInd].face[myDirInd] =
                        factorTo(probes[neighIndex], probe, neighDirInd, myDirInd);
                    neighFilter = glm::vec4(1.0f);
                    totalLeaving += factorTo(probe, probes[neighIndex], myDirInd, neighDirInd);
                }
            }

            probe.leavingPremulFactor.face[myDirInd] = 1.0f / totalLeaving;
        }
    }
}

} // namespace GI