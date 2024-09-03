#pragma once

#include "glm/glm.hpp"

namespace Vitrae
{

struct BoundingBox
{
    glm::vec3 min;
    glm::vec3 max;

    glm::vec3 getCenter() const;
    glm::vec3 getExtent() const;

    void merge(const BoundingBox &other);
};

} // namespace Vitrae