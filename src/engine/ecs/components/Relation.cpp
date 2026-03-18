#include "Relation.hpp"
#include "engine/ecs/World.hpp"

namespace ecs {

    std::pair<Entity, Entity> RelatedIteratorNonRecursive::operator*() const {
        return {parent, *current};
    }

    RelatedIteratorNonRecursive& RelatedIteratorNonRecursive::operator++() {
        ++current;
        return *this;
    }

    bool RelatedIteratorNonRecursive::operator!=(const RelatedIteratorNonRecursive& other) const {
        return current != other.current;
    }

    RelatedIteratorRecursive::RelatedIteratorRecursive(World *w, const Entity start, const ComponentID source_id) 
        : world(w), source_id(source_id), stack_size(0) {
        if (auto *source = static_cast<datastructures::EcsVec<Entity>*>(world->get_id(start, source_id))) {
            if (source->size > 0) {
                stack[stack_size++] = {start, source->begin(), source->end()};
            }
        }
    }

    RelatedIteratorRecursive::RelatedIteratorRecursive() : world(nullptr), source_id(0), stack_size(0) {
    }

    std::pair<Entity, Entity> RelatedIteratorRecursive::operator*() const {
        return {stack[stack_size - 1].parent, *(stack[stack_size - 1].current)};
    }

    RelatedIteratorRecursive& RelatedIteratorRecursive::operator++() {
        if (stack_size == 0) return *this;

        Level &top = stack[stack_size - 1];
        const Entity current_ent = *top.current;
        ++top.current;

        if (auto *source = static_cast<datastructures::EcsVec<Entity>*>(world->get_id(current_ent, source_id))) {
            if (source->size > 0 && stack_size < 64) {
                stack[stack_size++] = {current_ent, source->begin(), source->end()};
                return *this;
            }
        }

        while (stack_size > 0 && stack[stack_size - 1].current == stack[stack_size - 1].end) {
            stack_size--;
        }

        return *this;
    }

    bool RelatedIteratorRecursive::operator!=(const RelatedIteratorRecursive& other) const {
        return stack_size != other.stack_size;
    }

    RelatedIteratorNonRecursive RelatedRangeNonRecursive::begin() const {
        if (auto *source = static_cast<datastructures::EcsVec<Entity>*>(world->get_id(target, source_id))) {
            return RelatedIteratorNonRecursive{target, source->begin()};
        }
        return RelatedIteratorNonRecursive{target, nullptr};
    }

    RelatedIteratorNonRecursive RelatedRangeNonRecursive::end() const {
        if (auto *source = static_cast<datastructures::EcsVec<Entity>*>(world->get_id(target, source_id))) {
            return RelatedIteratorNonRecursive{target, source->end()};
        }
        return RelatedIteratorNonRecursive{target, nullptr};
    }

    RelatedIteratorRecursive RelatedRangeRecursive::begin() const {
        return RelatedIteratorRecursive(world, target, source_id);
    }

    RelatedIteratorRecursive RelatedRangeRecursive::end() const {
        return RelatedIteratorRecursive();
    }

}