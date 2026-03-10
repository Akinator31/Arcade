#pragma once

#include <unordered_map>

#include "../type.hpp"
#include "engine/datastructure/SparseSet.hpp"
#include "engine/reflection/TypeCounter.hpp"

struct EventCounter;

using EventType = reflection::TypeCounter<EventCounter>;

namespace ecs {
    class World;

    template<typename Data>
    struct EntityEvent {
        void (*callback)(World &, Entity, const Data) = nullptr;
    };
}

namespace ecs::internal {
    class EventRegistry {
        std::unordered_map<uint64_t, void *> entity_event_map;

        template<typename Event>
        static uint64_t id(const Entity entity) {
            return static_cast<uint64_t>(entity.index) << 32 | static_cast<uint64_t>(EventType::id<
                       Event>());
        }

    public:
        template<typename Event>
        void emit(World &world, const Entity entity, Event evt) {
            if (const uint64_t id = this->id<Event>(entity); this->entity_event_map.contains(id)) {
                auto *sys = static_cast<EntityEvent<Event> *>(this->entity_event_map.at(id));
                sys->callback(world, entity, evt);
            }
        }

        template<typename Event, typename Func>
            requires std::invocable<Func, World &, Entity, const Event>
        void listen(const Entity entity, Func &&func) {
            auto *evt = new EntityEvent<Event>();
            const uint64_t id = this->id<Event>(entity);

            evt->callback = func;

            this->entity_event_map[id] = static_cast<void *>(evt);
        }
    };
} // namespace ecs::internal
