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
Cpu structs
*/

using H_Transfer = glm::vec3; // per color
struct H_Source2FacesTransfer
{
    H_Transfer face[6]; // ind = to which face
};
struct H_NeighborTransfer
{
    H_Source2FacesTransfer source[6]; // ind = which neigh source
};

struct H_NeighborSpec
{
    std::uint32_t index;
    H_NeighborTransfer transfer;
};

struct H_ProbeDefinition
{
    glm::vec3 position;
    glm::vec3 size;
    H_Source2FacesTransfer reflectionTransfer;
    float leavingPremulFactor[6];
    std::vector<H_NeighborSpec> neighborSpecs;
};

struct H_ProbeState
{
    glm::vec3 illumination[6];
};

/*
Gpu structs
*/

using G_Transfer = glm::vec4; // per color
struct G_Source2FacesTransfer
{
    G_Transfer face[6]; // ind = to which face
};
struct G_NeighborTransfer
{
    G_Source2FacesTransfer source[6]; // ind = which neigh source
};

struct G_ProbeDefinition
{
    glm::vec4 position;
    glm::vec4 size;
    std::uint32_t neighborSpecBufStart;
    std::uint32_t neighborSpecCount;
};

struct G_ProbeState
{
    glm::vec4 illumination[6];
};

using ProbeBufferPtr = SharedBufferPtr<void, G_ProbeDefinition>;
using ProbeStateBufferPtr = SharedBufferPtr<void, G_ProbeState>;
using ReflectionBufferPtr = SharedBufferPtr<void, G_Source2FacesTransfer>;
using LeavingPremulFactorBufferPtr = SharedBufferPtr<void, float[6]>;
using NeighborIndexBufferPtr = SharedBufferPtr<void, std::uint32_t>;
using NeighborTransferBufferPtr = SharedBufferPtr<void, G_NeighborTransfer>;

inline constexpr const char *GLSL_PROBE_DEF_SNIPPET = R"(
const vec3 DIRECTIONS[6] = vec3[](
    vec3(1.0, 0.0, 0.0),  vec3(-1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0),
    vec3(0.0, -1.0, 0.0), vec3(0.0, 0.0, 1.0),  vec3(0.0, 0.0, -1.0)
);

struct Transfer {
    vec4 color;
};
struct Source2FacesTransfer {
    Transfer face[6];
};
struct NeighborTransfer {
    Source2FacesTransfer source[6];
};

struct ProbeDefinition {
    vec4 position;
    vec4 size;
    uint neighborSpecBufStart;
    uint neighborSpecCount;
};

)";

/*
Conversion
*/

void convertHost2GpuBuffers(std::span<const H_ProbeDefinition> hostProbes, ProbeBufferPtr gpuProbes,
                            ReflectionBufferPtr gpuReflectionTransfers,
                            LeavingPremulFactorBufferPtr gpuLeavingPremulFactors,
                            NeighborIndexBufferPtr gpuNeighborIndices,
                            NeighborTransferBufferPtr gpuNeighborTransfers);

} // namespace GI