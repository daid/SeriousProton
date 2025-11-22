#pragma once

#include <stdint.h>
#include <vector>
#include <limits>
#include "chunkedvector.h"


namespace sp {

// A sparseset is a more optimized version of a map<> with an important constrain:
//	The key has to be an integer type with a limited range, the smaller the better.
//	This gives optimized cache performance when iterating over all entities, but still allows quick lookup of individual entries.
template<typename T> class SparseSet final
{
public:
    bool has(uint32_t index)
    {
        if (index >= sparse.size())
            return false;
        return sparse[index] < dense.size();
    }
    
    T& get(uint32_t index)
    {
        return data[sparse[index]];
    }

    bool set(uint32_t index, const T& value)
    {
        if (has(index)) {
            data[sparse[index]] = value;
            return false;
        }
        if (sparse.size() <= index)
            sparse.resize(index + 1, std::numeric_limits<uint32_t>::max());
        if (free_dense != no_free_dense) {
            // Reuse a free slot
            auto new_free = dense[free_dense] & ~free_mark;
            sparse[index] = free_dense;
            dense[free_dense] = index;
            data[free_dense] = value;
            free_dense = new_free;
        } else {
            // Append to the data
            sparse[index] = static_cast<uint32_t>(dense.size());
            dense.push_back(index);
            data.emplace_back(value);
        }
        return true;
    }

    bool set(uint32_t index, T&& value)
    {
        if (has(index)) {
            data[sparse[index]] = std::move(value);
            return false;
        }
        if (sparse.size() <= index)
            sparse.resize(index + 1, std::numeric_limits<uint32_t>::max());
        if (free_dense != no_free_dense) {
            // Reuse a free slot
            auto new_free = dense[free_dense] & ~free_mark;
            sparse[index] = free_dense;
            dense[free_dense] = index;
            data[free_dense] = std::move(value);
            free_dense = new_free;
        } else {
            // Append to the data
            sparse[index] = static_cast<uint32_t>(dense.size());
            dense.push_back(index);
            data.emplace_back(std::move(value));
        }
        return true;
    }

    bool remove(uint32_t index)
    {
        if (!has(index))
            return false;
        auto new_free = sparse[index];
        sparse[index] = free_mark;
        dense[new_free] = free_dense | free_mark;
        free_dense = new_free;
        return true;
    }
    
    class Iterator
    {
    public:
        Iterator(SparseSet& _set, size_t _dense_index) : set(_set), dense_index(_dense_index) {
            if (_dense_index == 0) skipFree();
        }
        
        bool operator!=(const Iterator& other) const { return dense_index != other.dense_index; }
        void operator++() { dense_index++; skipFree(); }
        std::pair<uint32_t, T&> operator*() { return {set.dense[dense_index], set.data[dense_index]}; }
        std::pair<uint32_t, const T&> operator*() const { return {set.dense[dense_index], set.data[dense_index]}; }

        bool atEnd() { return dense_index >= set.dense.size(); }
    private:
        void skipFree() {
            while(dense_index < set.dense.size() && set.dense[dense_index] & free_mark)
                dense_index++;
        }

        SparseSet& set;
        size_t dense_index;
    };
    
    Iterator begin() { return Iterator(*this, 0); }
    Iterator end() { return Iterator(*this, dense.size()); }

    size_t size() { return data.size(); }
private:
    std::vector<uint32_t> sparse;
    std::vector<uint32_t> dense;
    ChunkedVector<T> data;
    static constexpr uint32_t free_mark = 0x80000000;
    static constexpr uint32_t no_free_dense = std::numeric_limits<uint32_t>::max() & ~free_mark;
    uint32_t free_dense = no_free_dense;
};

}