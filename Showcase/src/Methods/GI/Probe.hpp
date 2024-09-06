#pragma once

#include "Vitrae/Assets/SharedBuffer.hpp"
#include "Vitrae/Types/Typedefs.hpp"
#include "glm/glm.hpp"

#include <span>
#include <vector>

using namespace Vitrae;

namespace GI
{

inline constexpr glm::vec3 DIRECTIONS[] = {
    {1.0, 0.0, 0.0},  {-1.0, 0.0, 0.0}, {0.0, 1.0, 0.0},
    {0.0, -1.0, 0.0}, {0.0, 0.0, 1.0},  {0.0, 0.0, -1.0},
};

/*
Cpu and Gpu structs
*/

struct Reflection
{
    glm::vec4 face[6]; // ind = to which face
};
struct FaceTransfer
{
    float face[6]; // ind = to which face
};
struct NeighborTransfer
{
    FaceTransfer source[6]; // ind = which neigh source
};

/*
Cpu structs
*/

struct H_ProbeDefinition
{
    glm::vec3 position;
    glm::vec3 size;
    Reflection reflection;
    FaceTransfer leavingPremulFactor;
    std::vector<std::uint32_t> neighborIndices;
};

struct H_ProbeState
{
    glm::vec3 illumination[6];
};

/*
Gpu structs
*/

struct G_ProbeDefinition
{
    glm::vec3 position;
    std::uint32_t neighborSpecBufStart;
    glm::vec3 size;
    std::uint32_t neighborSpecCount;
};

struct G_ProbeState
{
    glm::vec4 illumination[6];
};

using ProbeBufferPtr = SharedBufferPtr<void, G_ProbeDefinition>;
using ProbeStateBufferPtr = SharedBufferPtr<void, G_ProbeState>;
using ReflectionBufferPtr = SharedBufferPtr<void, Reflection>;
using LeavingPremulFactorBufferPtr = SharedBufferPtr<void, FaceTransfer>;
using NeighborIndexBufferPtr = SharedBufferPtr<void, std::uint32_t>;
using NeighborTransferBufferPtr = SharedBufferPtr<void, NeighborTransfer>;
using NeighborFilterBufferPtr = SharedBufferPtr<void, glm::vec4>;

inline constexpr const char *GLSL_REFLECTION_DEF_SNIPPET = R"(
struct Reflection {
    float face[6];
};
)";

inline constexpr const char *GLSL_FACE_TRANSFER_DEF_SNIPPET = R"(
struct FaceTransfer {
    float face[6];
};
)";

inline constexpr const char *GLSL_NEIGHBOR_TRANSFER_DEF_SNIPPET = R"(
struct NeighborTransfer {
    FaceTransfer source[6];
};
)";

inline constexpr const char *GLSL_PROBE_DEF_SNIPPET = R"(
struct ProbeDefinition {
    vec3 position;
    uint neighborSpecBufStart;
    vec3 size;
    uint neighborSpecCount;
};
)";

inline constexpr const char *GLSL_PROBE_STATE_SNIPPET = R"(
struct ProbeState {
    vec4 illumination[6];
};
)";

inline constexpr const char *GLSL_PROBE_UTILITY_SNIPPET = R"(
const vec3 DIRECTIONS[6] = vec3[](
    vec3(1.0, 0.0, 0.0),  vec3(-1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0),
    vec3(0.0, -1.0, 0.0), vec3(0.0, 0.0, 1.0),  vec3(0.0, 0.0, -1.0)
);
)";

/*
Conversion
*/

void convertHost2GpuBuffers(std::span<const H_ProbeDefinition> hostProbes, ProbeBufferPtr gpuProbes,
                            ReflectionBufferPtr gpuReflectionTransfers,
                            LeavingPremulFactorBufferPtr gpuLeavingPremulFactors,
                            NeighborIndexBufferPtr gpuNeighborIndices,
                            NeighborTransferBufferPtr gpuNeighborTransfers,
                            NeighborFilterBufferPtr gpuNeighborFilters);

} // namespace GI