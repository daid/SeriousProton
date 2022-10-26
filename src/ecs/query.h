#pragma once

#include "ecs/entity.h"
#include "ecs/component.h"


namespace sp::ecs {

template<class PRIMARY, class... T> class Query {
public:
    class Iterator {
    public:
        Iterator(int) : iterator(ComponentStorage<PRIMARY>::storage.sparseset.begin()) { checkForSkip(); }
        Iterator() : iterator(ComponentStorage<PRIMARY>::storage.sparseset.end()) {}

        bool operator!=(const Iterator& other) const { return iterator != other.iterator; }
        void operator++() { ++iterator; checkForSkip(); }
        std::tuple<Entity, PRIMARY&, T&...> operator*() {
            auto [index, primary] = *iterator;
            return {Entity::fromIndex(index), primary, ComponentStorage<T>::storage.sparseset.get(index)...};
        }
    
    private:
        void checkForSkip() {
            if (iterator.atEnd()) return;
            if constexpr (sizeof...(T) > 0) {
                if (!checkIfHasAll<T...>()) {
                    ++iterator;
                }
            }
        }
        template<typename T2, typename... ARGS> bool checkIfHasAll() {
            auto index = (*iterator).first;
            if (!ComponentStorage<T2>::storage.sparseset.has(index))
                return false;
            if constexpr (sizeof...(ARGS) > 0)
                return checkIfHasAll<ARGS...>();
            return true;
        }

        typename SparseSet<PRIMARY>::Iterator iterator;
    };

    Iterator begin() {
        return Iterator(0);
    }

    Iterator end() {
        return Iterator();
    }
};

}