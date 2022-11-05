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
        sparse[index] = dense.size();
        dense.push_back(index);
        data.emplace_back(value);
        return true;
    }
    
    bool remove(uint32_t index)
    {
        if (!has(index))
            return false;
        uint32_t moved_index = dense.back();
        dense[sparse[index]] = moved_index;
        data[sparse[index]] = std::move(data.back());
        sparse[moved_index] = sparse[index];
        sparse[index] = std::numeric_limits<uint32_t>::max();
        
        dense.pop_back();
        data.pop_back();
        return true;
    }
    
    class Iterator
    {
    public:
        Iterator(SparseSet& _set, size_t _dense_index) : set(_set), dense_index(_dense_index) {}
        
        bool operator!=(const Iterator& other) const { return dense_index != other.dense_index; }
        void operator++() { dense_index--; }
        std::pair<uint32_t, T&> operator*() { return {set.dense[dense_index], set.data[dense_index]}; }
        std::pair<uint32_t, const T&> operator*() const { return {set.dense[dense_index], set.data[dense_index]}; }

        bool atEnd() { return dense_index == std::numeric_limits<size_t>::max(); }
    private:
        SparseSet& set;
        size_t dense_index;
    };
    
    Iterator begin() { return Iterator(*this, dense.size() - 1); }
    Iterator end() { return Iterator(*this, std::numeric_limits<size_t>::max()); }

    size_t size() { return data.size(); }
private:
    std::vector<uint32_t> sparse;
    std::vector<uint32_t> dense;
    ChunkedVector<T> data;
};

}