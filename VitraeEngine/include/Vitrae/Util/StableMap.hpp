#pragma once

#include <algorithm>
#include <cstddef>
#include <map>
#include <span>
#include <utility>
#include <vector>

namespace Vitrae
{

/**
 * @brief A map that expects its keys to be searched through often, but rarely changed
 * @note Modifying the values is fast
 *
 * @tparam KeyT the type of the key
 * @tparam MappedT the type of the value
 * @tparam CompT the comparison functor type
 */
template <class KeyT, class MappedT> class StableMap
{
    template <class KeyRefT, class MappedRefT> class AbstractStableMapIterator
    {
        friend class StableMap;

        KeyRefT *mp_key;
        MappedRefT *mp_value;

        AbstractStableMapIterator(KeyRefT *key, MappedRefT *value) : mp_key(key), mp_value(value) {}

      public:
        using iterator_category = std::random_access_iterator_tag;

        using value_type = std::pair<KeyRefT &, MappedRefT &>;

        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using allocator_type = std::allocator<std::byte>;

        using pointer = std::pair<KeyRefT *, MappedRefT *>;
        using reference = std::pair<KeyRefT &, MappedRefT &>;

        AbstractStableMapIterator() : mp_key(nullptr), mp_value(nullptr) {}
        AbstractStableMapIterator(const AbstractStableMapIterator &) = default;
        AbstractStableMapIterator(AbstractStableMapIterator &&) = default;

        AbstractStableMapIterator &operator=(const AbstractStableMapIterator &other) = default;
        AbstractStableMapIterator &operator=(AbstractStableMapIterator &&other) = default;

        auto operator++()
        {
            mp_key++;
            mp_value++;
            return *this;
        }
        auto operator--()
        {
            mp_key--;
            mp_value--;
            return *this;
        }

        auto operator++(int)
        {
            auto tmp = *this;
            mp_key++;
            mp_value++;
            return tmp;
        }
        auto operator--(int)
        {
            auto tmp = *this;
            mp_key--;
            mp_value--;
            return tmp;
        }

        auto operator+=(difference_type n)
        {
            mp_key += n;
            mp_value += n;
            return *this;
        }
        auto operator-=(difference_type n)
        {
            mp_key -= n;
            mp_value -= n;
            return *this;
        }

        auto operator+(difference_type n)
        {
            return AbstractStableMapIterator(mp_key + n, mp_value + n);
        }
        auto operator-(difference_type n)
        {
            return AbstractStableMapIterator(mp_key - n, mp_value - n);
        }
        friend auto operator+(difference_type n, const AbstractStableMapIterator &it)
        {
            return it + n;
        }
        friend auto operator-(difference_type n, const AbstractStableMapIterator &it)
        {
            return it - n;
        }

        auto operator-(const AbstractStableMapIterator &other) { return mp_key - other.mp_key; }

        auto operator==(const AbstractStableMapIterator &other) const
        {
            return mp_key == other.mp_key;
        }
        auto operator<=>(const AbstractStableMapIterator &other) const
        {
            return mp_key <=> other.mp_key;
        }

        auto operator*() const { return value_type(*mp_key, *mp_value); }
        auto operator->() const { return value_type(*mp_key, *mp_value); }
    };

  public:
    using StableMapIterator = AbstractStableMapIterator<const KeyT, MappedT>;
    using CStableMapIterator = AbstractStableMapIterator<const KeyT, const MappedT>;

    using key_type = KeyT;
    using mapped_type = MappedT;
    using value_type = StableMapIterator::value_type;
    using const_value_type = CStableMapIterator::value_type;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using allocator_type = std::allocator<std::byte>;

    using pointer = StableMapIterator::pointer;
    using const_pointer = CStableMapIterator::pointer;
    using reference = StableMapIterator::reference;
    using const_reference = CStableMapIterator::reference;
    using iterator = StableMapIterator;
    using const_iterator = CStableMapIterator;

    StableMap() : m_data(nullptr), m_size(0) {};

    StableMap(const StableMap &o)
    {
        m_size = o.m_size;
        m_data = new std::byte[getBufferSize(m_size)];
        KeyT *keyList = reinterpret_cast<KeyT *>(m_data);
        const KeyT *oKeyList = reinterpret_cast<const KeyT *>(o.m_data);
        m_valueList = reinterpret_cast<MappedT *>(m_data + getValueBufferOffset(m_size));
        for (std::size_t i = 0; i < m_size; ++i) {
            new (keyList + i) KeyT(oKeyList[i]);
            new (m_valueList + i) MappedT(o.m_valueList[i]);
        }
    }

    StableMap(StableMap &&o) : m_data(o.m_data), m_size(o.m_size), m_valueList(o.m_valueList)
    {
        o.m_size = 0;
        o.m_data = nullptr;
    }

    template <class InputItT>
    StableMap(InputItT first, InputItT last)
        requires requires(InputItT it) {
            { (*it).first } -> std::convertible_to<KeyT>;
            { (*it).second } -> std::convertible_to<MappedT>;
        }
    {
        std::size_t i;
        m_size = std::distance(first, last);
        m_data = new std::byte[getBufferSize(m_size)];
        KeyT *keyList = reinterpret_cast<KeyT *>(m_data);
        m_valueList = reinterpret_cast<MappedT *>(m_data + getValueBufferOffset(m_size));

        std::vector<InputItT> sortedIterators;
        sortedIterators.reserve(m_size);
        i = 0;
        for (auto it = first; it != last; ++it) {
            sortedIterators.emplace_back(it);
            ++i;
        }
        std::sort(sortedIterators.begin(), sortedIterators.end(),
                  [&](auto a, auto b) { return (*a).first < (*b).first; });

        i = 0;
        for (auto it : sortedIterators) {
            new (keyList + i) KeyT(it->first);
            new (m_valueList + i) MappedT(it->second);
            ++i;
        }
    }

    StableMap(std::initializer_list<std::pair<KeyT, MappedT>> initList)
        : StableMap(initList.begin(), initList.end())
    {}

    template <class OKeyT, class OMappedT>
    StableMap(std::map<OKeyT, OMappedT> orderedList)
        requires std::convertible_to<OKeyT, KeyT> && std::convertible_to<OMappedT, MappedT>
    {
        m_size = orderedList.size();
        m_data = new std::byte[getBufferSize(m_size)];
        KeyT *keyList = reinterpret_cast<KeyT *>(m_data);
        m_valueList = reinterpret_cast<MappedT *>(m_data + getValueBufferOffset(m_size));

        int i = 0;
        for (const auto &keyVal : orderedList) {
            new (keyList + i) KeyT(keyVal.first);
            new (m_valueList + i) MappedT(keyVal.second);
            ++i;
        }
    }

    ~StableMap()
    {
        KeyT *keyList = reinterpret_cast<KeyT *>(m_data);
        for (std::size_t i = 0; i < m_size; ++i) {
            keyList[i].~KeyT();
            m_valueList[i].~MappedT();
        }
        delete[] m_data;
    }

    StableMap &operator=(const StableMap &o)
    {
        m_size = o.m_size;
        m_data = new std::byte[getBufferSize(m_size)];
        KeyT *keyList = reinterpret_cast<KeyT *>(m_data);
        const KeyT *oKeyList = reinterpret_cast<const KeyT *>(o.m_data);
        m_valueList = reinterpret_cast<MappedT *>(m_data + getValueBufferOffset(m_size));
        for (std::size_t i = 0; i < m_size; ++i) {
            new (keyList + i) KeyT(oKeyList[i]);
            new (m_valueList + i) MappedT(o.m_valueList[i]);
        }
    }
    StableMap &operator=(StableMap &&o)
    {
        m_size = o.m_size;
        m_data = o.m_data;
        m_valueList = o.m_valueList;
        o.m_size = 0;
        o.m_data = nullptr;
        return *this;
    }

    std::size_t size() const { return m_size; }
    std::span<const KeyT> keys() const { return std::span<const KeyT>(getKeyList(), m_size); }
    std::span<MappedT> values() { return std::span<MappedT>(m_valueList, m_size); }
    std::span<const MappedT> values() const
    {
        return std::span<const MappedT>(m_valueList, m_size);
    }

    StableMapIterator begin() { return StableMapIterator(getKeyList(), m_valueList); }
    StableMapIterator end()
    {
        return StableMapIterator(getKeyList() + m_size, m_valueList + m_size);
    }
    CStableMapIterator begin() const { return CStableMapIterator(getKeyList(), m_valueList); }
    CStableMapIterator end() const
    {
        return CStableMapIterator(getKeyList() + m_size, m_valueList + m_size);
    }
    CStableMapIterator cbegin() const { return CStableMapIterator(getKeyList(), m_valueList); }
    CStableMapIterator cend() const
    {
        return CStableMapIterator(getKeyList() + m_size, m_valueList + m_size);
    }

    MappedT &operator[](const KeyT &key)
    {
        std::size_t ind;
        if (m_size > 0) {
            ind = findClosestIndex(key);
            if (ind < m_size && getKeyList()[ind] == key) {
                return m_valueList[ind];
            }
        } else {
            ind = 0;
        }

        realloc_w_uninit(ind);
        new (getKeyList() + ind) KeyT(key);
        new (m_valueList + ind) MappedT();
        return m_valueList[ind];
    }
    const MappedT &operator[](const KeyT &key) const { return at(key); }

    MappedT &at(const KeyT &key)
    {
        if (m_size > 0) {
            std::size_t ind = findClosestIndex(key);
            if (ind < m_size && !(key < getKeyList()[ind])) {
                return m_valueList[ind];
            }
        }
        throw std::out_of_range("Key not found");
    }

    const MappedT &at(const KeyT &key) const
    {
        if (m_size > 0) {
            std::size_t ind = findClosestIndex(key);
            if (ind < m_size && !(key < getKeyList()[ind])) {
                return m_valueList[ind];
            }
        }
        throw std::out_of_range("Key not found");
    }

    template <class... Args> std::pair<iterator, bool> emplace(const KeyT &key, Args &&...args)
    {
        std::size_t ind;
        if (m_size > 0) {
            ind = findClosestIndex(key);
            if (ind < m_size && !(key < getKeyList()[ind])) {
                return std::make_pair(iterator(getKeyList() + ind, m_valueList + ind), false);
            }
        } else {
            ind = 0;
        }

        realloc_w_uninit(ind);

        // insert
        new (getKeyList() + ind) KeyT(key);
        new (m_valueList + ind) MappedT(std::forward<Args>(args)...);

        return std::make_pair(iterator(getKeyList() + ind, m_valueList + ind), true);
    }

    std::size_t erase(const KeyT &key)
    {
        if (m_size > 0) {
            std::size_t ind = findClosestIndex(key);
            if (ind < m_size && !(key < getKeyList()[ind])) {
                realloc_w_erased(ind);
                return 1;
            }
        }

        return 0;
    }

  protected:
    std::size_t m_size;
    std::byte *m_data;    // starts with keys, also contains values. Owned by the map
    MappedT *m_valueList; // pointer to a portion of m_data buffer

    static constexpr std::size_t getValueBufferOffset(std::size_t numElements)
    {
        return (alignof(MappedT) <= alignof(KeyT))
                   ? numElements * sizeof(KeyT)
                   : (numElements * sizeof(KeyT) + alignof(MappedT) - 1) / alignof(MappedT) *
                         alignof(MappedT);
    }

    static constexpr std::size_t getBufferSize(std::size_t numElements)
    {
        return getValueBufferOffset(numElements) + numElements * sizeof(MappedT);
    }

    std::size_t findClosestIndex(const KeyT &key) const
    {
        // uses binary search to find the index of the closest key
        const KeyT *keyList = getKeyList();
        std::size_t leftIndex = 0;
        std::size_t rightIndex = m_size;
        while (leftIndex < rightIndex) {
            std::size_t midIndex = (leftIndex + rightIndex) / 2;
            if (keyList[midIndex] < key) {
                leftIndex = midIndex + 1;
            } else {
                rightIndex = midIndex;
            }
        }
        return leftIndex;
    }

    KeyT *getKeyList() { return reinterpret_cast<KeyT *>(m_data); }
    const KeyT *getKeyList() const { return reinterpret_cast<const KeyT *>(m_data); }

    void realloc_buf(std::size_t newSize)
    {
        KeyT *keyList = reinterpret_cast<KeyT *>(m_data);

        std::byte *newData = new std::byte[getBufferSize(newSize)];
        KeyT *newKeyList = reinterpret_cast<KeyT *>(newData);
        MappedT *newValueList =
            reinterpret_cast<MappedT *>(newData + getValueBufferOffset(newSize));

        if (newSize > m_size) {
            for (std::size_t i = 0; i < m_size; ++i) {
                new (newKeyList + i) KeyT(keyList[i]);
                new (newValueList + i) MappedT(m_valueList[i]);
                keyList[i].~KeyT();
                m_valueList[i].~MappedT();
            }
            for (std::size_t i = m_size; i < newSize; ++i) {
                new (newKeyList + i) KeyT();
                new (newValueList + i) MappedT();
            }
        } else {
            for (std::size_t i = 0; i < newSize; ++i) {
                new (newKeyList + i) KeyT(keyList[i]);
                new (newValueList + i) MappedT(m_valueList[i]);
                keyList[i].~KeyT();
                m_valueList[i].~MappedT();
            }
            for (std::size_t i = newSize; i < m_size; ++i) {
                keyList[i].~KeyT();
                m_valueList[i].~MappedT();
            }
        }

        delete[] m_data;
        m_data = newData;
        m_valueList = newValueList;
        m_size = newSize;
    }

    void realloc_w_erased(difference_type erasingIndex)
    {
        KeyT *keyList = reinterpret_cast<KeyT *>(m_data);

        std::byte *newData = new std::byte[getBufferSize(m_size - 1)];
        KeyT *newKeyList = reinterpret_cast<KeyT *>(newData);
        MappedT *newValueList =
            reinterpret_cast<MappedT *>(newData + getValueBufferOffset(m_size - 1));

        // move data before
        for (std::size_t i = 0; i < erasingIndex; ++i) {
            new (newKeyList + i) KeyT(std::move(keyList[i]));
            new (newValueList + i) MappedT(std::move(m_valueList[i]));
            keyList[i].~KeyT();
            m_valueList[i].~MappedT();
        }

        // erase
        keyList[erasingIndex].~KeyT();
        m_valueList[erasingIndex].~MappedT();

        // move data after
        for (std::size_t i = erasingIndex + 1; i < m_size; ++i) {
            new (newKeyList + i - 1) KeyT(std::move(keyList[i]));
            new (newValueList + i - 1) MappedT(std::move(m_valueList[i]));
            keyList[i].~KeyT();
            m_valueList[i].~MappedT();
        }

        delete[] m_data;
        m_data = newData;
        m_valueList = newValueList;
        --m_size;
    }

    void realloc_w_uninit(difference_type uninitIndex)
    {
        KeyT *keyList = reinterpret_cast<KeyT *>(m_data);

        std::size_t newBufferSize = getBufferSize(m_size + 1);
        std::byte *newData = new std::byte[newBufferSize];
        KeyT *newKeyList = reinterpret_cast<KeyT *>(newData);
        MappedT *newValueList =
            reinterpret_cast<MappedT *>(newData + getValueBufferOffset(m_size + 1));

        // move data before
        for (std::size_t i = 0; i < uninitIndex; ++i) {
            new (newKeyList + i) KeyT(std::move(keyList[i]));
            new (newValueList + i) MappedT(std::move(m_valueList[i]));
            keyList[i].~KeyT();
            m_valueList[i].~MappedT();
        }

        // move data after
        for (std::size_t i = uninitIndex; i < m_size; ++i) {
            new (newKeyList + i + 1) KeyT(std::move(keyList[i]));
            new (newValueList + i + 1) MappedT(std::move(m_valueList[i]));
            keyList[i].~KeyT();
            m_valueList[i].~MappedT();
        }

        delete[] m_data;
        m_data = newData;
        m_valueList = newValueList;
        ++m_size;
    }
};

} // namespace Vitrae