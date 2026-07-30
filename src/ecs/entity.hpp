#pragma once

#include <cstdint>
#include <vector>

struct Entity {
    uint32_t index;
    uint32_t generation;

    bool operator==(const Entity&) const = default;
};

class EntityManager {
    private:
        std::vector<uint32_t> generations;
        std::vector<bool> alive;
        std::vector<uint32_t> free_indices;

    public:
        Entity create();
        void destroy(Entity entity);
        bool is_alive(Entity entity) const;
};
