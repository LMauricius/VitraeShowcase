#include "Generation.hpp"

namespace GI
{

namespace
{

float factorTo(const H_ProbeDefinition &srcProbe, const H_ProbeDefinition &dstProbe,
               int srcDirIndex, int dstDirIndex)
{}
} // namespace

const char *const GLSL_PROBE_GEN_SNIPPET = R"(
)";

void generateProbeList(std::vector<H_ProbeDefinition> &probes, glm::vec3 worldCenter,
                       glm::vec3 worldSize, float minProbeSize)
{
    glm::ivec3 gridSize = glm::ivec3(worldSize / minProbeSize);

    probes.resize(gridSize.x * gridSize.y * gridSize.z);

    auto getIndex = [&](glm::ivec3 ind) -> std::uint32_t {
        return ind.x * gridSize.y * gridSize.z + ind.y * gridSize.z + ind.z;
    };
    auto getProbe = [&](glm::ivec3 ind) -> auto & { return probes[getIndex(ind)]; };

    // edge probes' centers are put at world edges
    glm::vec3 probeSize = worldSize / glm::vec3(gridSize * 2 - 2) * 2.0f;
    glm::vec3 worldStart = worldCenter - worldSize / 2.0f;
    glm::ivec3 minIndex = glm::ivec3(0, 0, 0);
    glm::ivec3 maxIndex = gridSize - 1;
    glm::vec3 ind2OffsetConversion = worldSize / glm::vec3(maxIndex);

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

} // namespace GI