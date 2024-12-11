#pragma once

#include <stdint.h>
#include <vector>


namespace sp {

class Bitset final
{
public:
    bool has(uint32_t index)
    {
        auto idx = storageIndex(index);
        if (idx >= storage.size()) return false;
        return storage[idx] & storageMask(index);
    }

    void set(uint32_t index)
    {
        auto idx = storageIndex(index);
        if (idx >= storage.size()) storage.resize(idx + 1, 0);
        storage[idx] |= storageMask(index);
    }

    void reset(uint32_t index)
    {
        auto idx = storageIndex(index);
        if (idx >= storage.size()) return;
        storage[idx] &=~storageMask(index);
    }

    void clear(uint32_t index)
    {
        storage.clear();
    }
private:
    using StorageType = uint32_t;
    size_t storageIndex(uint32_t index) { return index / (8 * sizeof(StorageType));}
    StorageType storageMask(uint32_t index) { return 1 << (index % (8 * sizeof(StorageType)));}

    std::vector<StorageType> storage;
};

}