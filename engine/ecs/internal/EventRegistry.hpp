#pragma once

#include <ranges>
#include <unordered_map>

#include "../type.hpp"
#include "engine/datastructures/SparseSet.hpp"
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
    struct EventRegistry {
        struct EventRecord {
            void *instance = nullptr;
            void (*destroy)(void *) = nullptr;
        };

        std::unordered_map<uint64_t, EventRecord> entity_event_map;

        virtual ~EventRegistry() {
            for (auto &[instance, destroy]: entity_event_map | std::views::values) {
                if (destroy) {
                    destroy(instance);
                }
            }
        }

        template<typename Event>
        static uint64_t id(const Entity entity) {
            return static_cast<uint64_t>(entity.index) << 32 | static_cast<uint64_t>(EventType::id<
                       Event>());
        }

        template<typename Event, typename Func>
            requires std::invocable<Func, World &, Entity, const Event>
        void listen(const Entity entity, Func &&func) {
            const uint64_t id = this->id<Event>(entity);
            auto *evt = new EntityEvent<Event>();
            evt->callback = func;
            if (const auto it = this->entity_event_map.find(id); it != this->entity_event_map.end()) {
                if (it->second.destroy) {
                    it->second.destroy(it->second.instance);
                }
            }
            this->entity_event_map[id] = {
                static_cast<void *>(evt),
                [](void *ptr) { delete static_cast<EntityEvent<Event> *>(ptr); }
            };
        }
    };
} // namespace ecs::internal
