#include "ecs/world.hpp"

#include <cassert>

Entity EntityManager::create() {
    if (!this->free_indices.empty()) {
        uint32_t index = this->free_indices.back();
        this->free_indices.pop_back();
        this->alive[index] = true;
        return Entity{index, this->generations[index]};
    }

    uint32_t index = static_cast<uint32_t>(this->generations.size());
    this->generations.push_back(0);
    this->alive.push_back(true);
    return Entity{index, 0};
}

void EntityManager::destroy(Entity entity) {
    assert(this->is_alive(entity));
    this->alive[entity.index] = false;
    this->generations[entity.index]++;
    this->free_indices.push_back(entity.index);
}

bool EntityManager::is_alive(Entity entity) const {
    return entity.index < this->generations.size() && this->alive[entity.index] &&
        this->generations[entity.index] == entity.generation;
}

Entity World::create_package(float x, float y, std::string name) {
    Entity entity = this->entity_manager.create();
    this->transforms.emplace(entity, Transform{x, y});
    this->packages.emplace(entity, Package{std::move(name)});
    return entity;
}

void World::destroy(Entity entity) {
    assert(this->is_alive(entity));
    this->transforms.remove(entity);
    this->packages.remove(entity);
    this->silkscreens.remove(entity);
    this->entity_manager.destroy(entity);
}

bool World::is_alive(Entity entity) const {
    return this->entity_manager.is_alive(entity);
}

Transform& World::transform(Entity entity) {
    assert(this->is_alive(entity));
    return this->transforms.get(entity);
}

const Transform& World::transform(Entity entity) const {
    assert(this->is_alive(entity));
    return this->transforms.get(entity);
}

Package& World::package(Entity entity) {
    assert(this->is_alive(entity));
    return this->packages.get(entity);
}

const Package& World::package(Entity entity) const {
    assert(this->is_alive(entity));
    return this->packages.get(entity);
}

bool World::has_silkscreen(Entity entity) const {
    return this->is_alive(entity) && this->silkscreens.has(entity);
}

Silkscreen& World::add_silkscreen(Entity entity, std::string text) {
    assert(this->is_alive(entity));
    if (this->silkscreens.has(entity)) {
        this->silkscreens.get(entity).text = std::move(text);
        return this->silkscreens.get(entity);
    }
    return this->silkscreens.emplace(entity, Silkscreen{std::move(text)});
}

const Silkscreen& World::silkscreen(Entity entity) const {
    assert(this->is_alive(entity));
    return this->silkscreens.get(entity);
}

void World::remove_silkscreen(Entity entity) {
    assert(this->is_alive(entity));
    this->silkscreens.remove(entity);
}

const std::vector<Entity>& World::package_entities() const {
    return this->packages.entities();
}
