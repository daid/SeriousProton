#pragma once

#include "ecs/entity.h"
#include "ecs/component.h"


namespace sp::ecs {

template<typename T> struct optional {};

template<typename T> struct optional_info {
    using base_type = T;
    using ref_type = T&;
    static constexpr bool value = false;
};
template<typename T> struct optional_info<optional<T>> {
    using base_type = T;
    using ref_type = T*;
    static constexpr bool value = true;
};

template<class PRIMARY, class... T> class Query {
public:
    class Iterator {
    public:
        Iterator(int) : iterator(ComponentStorage<PRIMARY>::storage.sparseset.begin()) { while(checkForSkip()) {} }
        Iterator() : iterator(ComponentStorage<PRIMARY>::storage.sparseset.end()) {}

        bool operator!=(const Iterator& other) const { return iterator != other.iterator; }
        void operator++() { ++iterator; while(checkForSkip()) {} }
        std::tuple<Entity, PRIMARY&, typename optional_info<T>::ref_type...> operator*() {
            auto [index, primary] = *iterator;
            return {Entity::fromIndex(index), primary, getComponent<T>(index)...};
        }
    
    private:
        template<typename T2> typename optional_info<T2>::ref_type getComponent(uint32_t index)
        {
            if constexpr (optional_info<T2>::value)
            {
                if (!ComponentStorage<typename optional_info<T2>::base_type>::storage.sparseset.has(index))
                    return nullptr;
                return &ComponentStorage<typename optional_info<T2>::base_type>::storage.sparseset.get(index);
            } else {
                return ComponentStorage<T2>::storage.sparseset.get(index);
            }
        }

        bool checkForSkip() {
            if (iterator.atEnd()) return false;
            if constexpr (sizeof...(T) > 0) {
                if (!checkIfHasAll<T...>()) {
                    ++iterator;
                    return true;
                }
            }
            return false;
        }
        template<typename T2, typename... ARGS> bool checkIfHasAll() {
            if constexpr (!optional_info<T2>::value) {
                auto index = (*iterator).first;
                if (!ComponentStorage<T2>::storage.sparseset.has(index))
                    return false;
            }
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