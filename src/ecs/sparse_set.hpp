#pragma once

#include <cassert>
#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

#include "ecs/entity.hpp"

template <typename T>
class SparseSet {
    private:
        static constexpr size_t missing = std::numeric_limits<size_t>::max();

        std::vector<size_t> sparse;
        std::vector<Entity> dense_entities;
        std::vector<T> dense_components;

        size_t dense_index(Entity entity) const {
            if (entity.index >= this->sparse.size()) {
                return missing;
            }

            size_t index = this->sparse[entity.index];
            if (index == missing || this->dense_entities[index] != entity) {
                return missing;
            }
            return index;
        }

    public:
        bool has(Entity entity) const {
            return this->dense_index(entity) != missing;
        }

        template <typename... Args>
        T& emplace(Entity entity, Args&&... args) {
            assert(!this->has(entity));
            if (entity.index >= this->sparse.size()) {
                this->sparse.resize(entity.index + 1, missing);
            }

            size_t index = this->dense_components.size();
            this->sparse[entity.index] = index;
            this->dense_entities.push_back(entity);
            this->dense_components.emplace_back(std::forward<Args>(args)...);
            return this->dense_components.back();
        }

        T& get(Entity entity) {
            size_t index = this->dense_index(entity);
            assert(index != missing);
            return this->dense_components[index];
        }

        const T& get(Entity entity) const {
            size_t index = this->dense_index(entity);
            assert(index != missing);
            return this->dense_components[index];
        }

        void remove(Entity entity) {
            size_t index = this->dense_index(entity);
            if (index == missing) {
                return;
            }

            size_t last_index = this->dense_components.size() - 1;
            if (index != last_index) {
                this->dense_components[index] = std::move(this->dense_components[last_index]);
                this->dense_entities[index] = this->dense_entities[last_index];
                this->sparse[this->dense_entities[index].index] = index;
            }

            this->dense_components.pop_back();
            this->dense_entities.pop_back();
            this->sparse[entity.index] = missing;
        }

        const std::vector<Entity>& entities() const {
            return this->dense_entities;
        }
};
