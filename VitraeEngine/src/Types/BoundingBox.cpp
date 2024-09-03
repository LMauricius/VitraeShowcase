#include "Vitrae/Types/BoundingBox.hpp"

namespace Vitrae
{

glm::vec3 Vitrae::BoundingBox::getCenter() const
{
    return (min + max) / 2.0f;
}

glm::vec3 Vitrae::BoundingBox::getExtent() const
{
    return max - min;
}

void Vitrae::BoundingBox::merge(const BoundingBox &other)
{
    min = glm::min(min, other.min);
    max = glm::max(max, other.max);
}

} // namespace Vitrae