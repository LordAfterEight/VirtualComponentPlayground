#pragma once

#include <string>
#include <vector>

#include "ecs/components.hpp"
#include "ecs/entity.hpp"
#include "ecs/sparse_set.hpp"

class World {
    private:
        EntityManager entity_manager;
        SparseSet<Transform> transforms;
        SparseSet<Package> packages;
        SparseSet<Silkscreen> silkscreens;

    public:
        Entity create_package(float x, float y, std::string name);
        void destroy(Entity entity);
        bool is_alive(Entity entity) const;

        Transform& transform(Entity entity);
        const Transform& transform(Entity entity) const;
        Package& package(Entity entity);
        const Package& package(Entity entity) const;
        bool has_silkscreen(Entity entity) const;
        Silkscreen& add_silkscreen(Entity entity, std::string text);
        const Silkscreen& silkscreen(Entity entity) const;
        void remove_silkscreen(Entity entity);

        const std::vector<Entity>& package_entities() const;
};
