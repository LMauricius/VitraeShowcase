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

    glm::vec3 srcCenter = srcProbe.position;
    glm::vec3 wallCenter = dstProbe.position + DIRECTIONS[dstDirIndex] * dstProbe.size / 2.0f;
    float wallSize;
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
)";

void generateProbeList(std::vector<H_ProbeDefinition> &probes, glm::ivec3 &gridSize,
                       glm::vec3 &worldStart, glm::vec3 worldCenter, glm::vec3 worldSize,
                       float minProbeSize)
{
    MMETER_FUNC_PROFILER;

    gridSize = glm::ivec3(worldSize / minProbeSize);
    worldStart = worldCenter - worldSize / 2.0f;

    probes.resize(gridSize.x * gridSize.y * gridSize.z);

    auto getIndex = [gridSize](glm::ivec3 ind) -> std::uint32_t {
        return ind.x * gridSize.y * gridSize.z + ind.y * gridSize.z + ind.z;
    };
    auto getProbe = [&](glm::ivec3 ind) -> auto & { return probes[getIndex(ind)]; };

    // edge probes' centers are put at world edges
    glm::vec3 probeSize = worldSize / glm::vec3(gridSize * 2 - 2) * 2.0f;
    glm::ivec3 minIndex = glm::ivec3(0, 0, 0);
    glm::ivec3 maxIndex = gridSize - 1;
    glm::vec3 ind2OffsetConversion = worldSize / glm::vec3(maxIndex);

    {
        MMETER_SCOPE_PROFILER("Probe gen");

        for (int x = 0; x < gridSize.x; x++) {
            for (int y = 0; y < gridSize.y; y++) {
                for (int z = 0; z < gridSize.z; z++) {
                    auto &probe = getProbe({x, y, z});

                    probe.position = worldStart + glm::vec3(x, y, z) * ind2OffsetConversion;
                    probe.size = probeSize;

                    for (int neighx : {-1, 0, 1}) {
                        for (int neighy : {-1, 0, 1}) {
                            for (int neighz : {-1, 0, 1}) {
                                if ((neighx == 0 && neighy == 0 && neighz == 0) || x + neighx < 0 ||
                                    x + neighx >= maxIndex.x || y + neighy < 0 ||
                                    y + neighy >= maxIndex.y || z + neighz < 0 ||
                                    z + neighz >= maxIndex.z) {
                                    continue;
                                }
                                auto &neighProbe = getProbe({x + neighx, y + neighy, z + neighz});
                                probe.neighborSpecs.push_back({
                                    getIndex({x + neighx, y + neighy, z + neighz}),
                                    H_NeighborTransfer{},
                                });
                            }
                        }
                    }
                }
            }
        }
    }

    {
        MMETER_SCOPE_PROFILER("Transfer gen");

        for (int x = 0; x < gridSize.x; x++) {
            for (int y = 0; y < gridSize.y; y++) {
                for (int z = 0; z < gridSize.z; z++) {
                    auto &probe = getProbe({x, y, z});

                    for (int myDirInd = 0; myDirInd < 6; myDirInd++) {
                        float totalLeaving = 0.0;
                        for (int neighDirInd = 0; neighDirInd < 6; neighDirInd++) {
                            for (auto &neighSpec : probe.neighborSpecs) {
                                neighSpec.transfer.source[neighDirInd].face[myDirInd] =
                                    glm::vec3(1, 1, 1) *
                                    factorTo(probes[neighSpec.index], probe, neighDirInd, myDirInd);
                                totalLeaving +=
                                    factorTo(probe, probes[neighSpec.index], myDirInd, neighDirInd);
                            }
                        }

                        probe.leavingPremulFactor[myDirInd] = 1.0f / totalLeaving;
                    }
                }
            }
        }
    }
}

} // namespace GI