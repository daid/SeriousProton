#pragma once

#include <stdint.h>
#include <vector>
#include <limits>
#include <memory>


namespace sp {

template<typename T, size_t CHUNK_SIZE=128> class ChunkedVector final
{
public:
    size_t size() {
        return count;
    }

    template<typename... ARGS> void emplace_back(ARGS&&... args) {
        if (count == chunks.size() * CHUNK_SIZE)
            chunks.push_back(new Chunk());
        auto ptr = &(*this)[count];
        new (ptr)T(std::forward<ARGS>(args)...);
        count++;
    }
    void pop_back() {
        count -= 1;
        (&(*this)[count])->~T();
    }

    T& operator[](size_t index) {
        auto& chunk = chunks[index / CHUNK_SIZE];
        auto& storage = chunk->elements[index % CHUNK_SIZE];
        return *((T*)&storage);
    }
    const T& operator[](size_t index) const {
        auto& chunk = chunks[index / CHUNK_SIZE];
        auto& storage = chunk->elements[index % CHUNK_SIZE];
        return *((const T*)&storage);
    }

    T& back() {
        return (*this)[count - 1];
    }
    const T& back() const {
        return (*this)[count - 1];
    }
private:
    using Storage = uint8_t[sizeof(T)];
    struct Chunk {
        Storage elements[CHUNK_SIZE];
    };

    size_t count = 0;
    std::vector<Chunk*> chunks;
};

}