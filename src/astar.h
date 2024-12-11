#pragma once

#include <functional>
#include <vector>
#include <queue>


template<typename T> std::vector<T> astar(
    T start, T target,
    std::vector<std::pair<T, float>>(*get_neighbors)(T),
    float(*get_distance)(T, T)) {

    using queueItem = std::pair<int, T>;
    auto cmp = [](queueItem left, queueItem right) { return left.first > right.first; };
    std::priority_queue<queueItem, std::vector<queueItem>, decltype(cmp)> todo{cmp};

    std::unordered_map<T, float> cost_so_far;
    std::unordered_map<T, T> came_from;
    cost_so_far[start] = 0;

    todo.push({0, start});
    while(!todo.empty()) {
        auto [priority, position] = todo.top();
        todo.pop();
        if (position == target)
            break;
        for(auto [pos, c] : get_neighbors(position)) {
            auto new_cost = cost_so_far[position] + c;
            if (cost_so_far.find(pos) == cost_so_far.end() || new_cost < cost_so_far[pos]) {
                cost_so_far[pos] = new_cost;
                priority = new_cost + get_distance(pos, target);
                todo.push({priority, pos});
                came_from[pos] = position;
            }
        }
    }

    std::vector<T> path;
    while(true) {
        if (came_from.find(target) == came_from.end()) {
            std::reverse(path.begin(), path.end());
            return path;
        }
        path.push_back(target);
        target = came_from[target];
    }
}
