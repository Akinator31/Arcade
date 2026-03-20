#pragma once

#include <algorithm>
#include <cstdint>
#include <ranges>
#include <unordered_map>
#include <vector>

#include "../../entity/type.hpp"
#include "engine/reflection/TypeCounter.hpp"

struct EventCounter;

using EventType = reflection::TypeCounter<EventCounter>;
using EventListenerId = uint64_t;

namespace ecs {
    class World;

    template<typename Data>
    struct EntityEvent {
        struct Listener {
            EventListenerId id;
            std::function<void (World&, Entity, Data)> callback;
        };

        std::vector<Listener> listeners;
        EventListenerId next_listener_id = 1;
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
        EventListenerId listen(const Entity entity, Func &&func) {
            const uint64_t id = this->id<Event>(entity);

            EntityEvent<Event> *evt = nullptr;
            if (const auto it = this->entity_event_map.find(id); it != this->entity_event_map.end()) {
                evt = static_cast<EntityEvent<Event> *>(it->second.instance);
            } else {
                evt = new EntityEvent<Event>();
                this->entity_event_map[id] = {
                    static_cast<void *>(evt),
                    [](void *ptr) { delete static_cast<EntityEvent<Event> *>(ptr); }
                };
            }

            const EventListenerId listener_id = evt->next_listener_id++;
            evt->listeners.push_back({listener_id, std::forward<Func>(func)});
            return listener_id;
        }

        template<typename Event>
        void unlisten(const Entity entity) {
            const uint64_t id = this->id<Event>(entity);
            if (const auto it = this->entity_event_map.find(id); it != this->entity_event_map.end()) {
                if (it->second.destroy) {
                    it->second.destroy(it->second.instance);
                }
                this->entity_event_map.erase(it);
            }
        }

        template<typename Event>
        void unlisten(const Entity entity, const EventListenerId listener_id) {
            const uint64_t id = this->id<Event>(entity);
            const auto it = this->entity_event_map.find(id);
            if (it == this->entity_event_map.end()) {
                return;
            }

            auto *evt = static_cast<EntityEvent<Event> *>(it->second.instance);
            evt->listeners.erase(
                std::remove_if(
                    evt->listeners.begin(),
                    evt->listeners.end(),
                    [listener_id](const typename EntityEvent<Event>::Listener &listener) {
                        return listener.id == listener_id;
                    }
                ),
                evt->listeners.end()
            );

            if (evt->listeners.empty()) {
                if (it->second.destroy) {
                    it->second.destroy(it->second.instance);
                }
                this->entity_event_map.erase(it);
            }
        }

        void unlistenAll(const Entity entity) {
            const uint64_t prefix = static_cast<uint64_t>(entity.index) << 32;

            for (auto it = this->entity_event_map.begin(); it != this->entity_event_map.end();) {
                if ((it->first & 0xFFFFFFFF00000000ULL) != prefix) {
                    ++it;
                    continue;
                }

                if (it->second.destroy) {
                    it->second.destroy(it->second.instance);
                }
                it = this->entity_event_map.erase(it);
            }
        }
    };
} // namespace ecs::internal
