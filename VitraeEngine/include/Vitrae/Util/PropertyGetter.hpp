#pragma once

#include "Vitrae/Util/ScopedDict.hpp"

#include <variant>

namespace Vitrae
{

/**
 * Object that either contains a fixed value, or references a property by its nameid.
 * It can dynamically retrieve the value if referenced by the name from the surrounding scope.
 */
template <class T> class PropertyGetter
{
    std::variant<StringId, T> m_nameOrValue;

  public:
    PropertyGetter(StringId nameId) : m_nameOrValue(std::in_place_index<0>, nameId) {}
    PropertyGetter(const T &value) : m_nameOrValue(std::in_place_index<1>, value) {}
    PropertyGetter(T &&value) : m_nameOrValue(std::in_place_index<1>, std::move(value)) {}

    PropertyGetter &operator=(StringId nameId)
    {
        m_nameOrValue.template emplace<0>(nameId);
        return *this;
    }
    PropertyGetter &operator=(const T &value)
    {
        m_nameOrValue.template emplace<1>(value);
        return *this;
    }

    T get(const ScopedDict &scope) const
    {
        switch (m_nameOrValue.index()) {
        case 0:
            return scope.get(std::get<0>(m_nameOrValue)).template get<T>();
        case 1:
        default:
            return std::get<1>(m_nameOrValue);
        }
    }

    T *getPtr(const ScopedDict &scope) const
    {
        switch (m_nameOrValue.index()) {
        case 0:
            const Variant *p = scope.getPtr(std::get<0>(m_nameOrValue));
            if (p)
                return &(p->get<T>());
            else
                return nullptr;
        case 1:
        default:
            return &(std::get<1>(m_nameOrValue));
        }
    }
};

} // namespace Vitrae