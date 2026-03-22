#pragma once

#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "EventRegistry.hpp"

struct GlobalEventCounter;

using GlobalEventType = reflection::TypeCounter<GlobalEventCounter>;

namespace ecs::internal {
    struct GlobalEventListener {
        bool alive = false;
        uint16_t type = 0;
        uint32_t owner = UINT32_MAX;
        uint32_t event_slot = 0;
        uint32_t owner_slot = 0;
        void *callback = nullptr;

        void (*run)(void *, ecs::World &, const void *) = nullptr;

        void (*destroy)(void *) = nullptr;
    };

    struct GlobalEventRegistry {
        static constexpr uint32_t no_owner = UINT32_MAX;

        std::vector<GlobalEventListener> listeners;
        std::unordered_map<uint16_t, std::vector<EventListenerId> > event_listeners;
        std::unordered_map<uint32_t, std::vector<EventListenerId> > owner_listeners;

        virtual ~GlobalEventRegistry() {
            for (const auto &listener: this->listeners) {
                if (listener.alive && listener.destroy) {
                    listener.destroy(listener.callback);
                }
            }
        }

        template<typename Event>
        static uint16_t type() {
            return GlobalEventType::id<Event>();
        }

        template<typename Event>
        void emit(World &world, Event evt) {
            const auto it = this->event_listeners.find(this->type<Event>());
            if (it == this->event_listeners.end()) {
                return;
            }

            for (const auto ids = it->second; const EventListenerId listener_id: ids) {
                GlobalEventListener *listener = this->getListener(listener_id);
                if (listener == nullptr || listener->type != this->type<Event>() || listener->run == nullptr) {
                    continue;
                }
                listener->run(listener->callback, world, &evt);
            }
        }

        template<typename Event, typename Func>
            requires std::invocable<Func, ecs::World &, const Event>
        EventListenerId listen(Func &&func) {
            return this->add<Event>(no_owner, std::forward<Func>(func));
        }

        template<typename Event, typename Func>
            requires std::invocable<Func, ecs::World &, const Event>
        EventListenerId listen(const ecs::Entity owner, Func &&func) {
            return this->add<Event>(owner.index, std::forward<Func>(func));
        }

        template<typename Event>
        void unlisten() {
            const auto it = this->event_listeners.find(this->type<Event>());
            if (it == this->event_listeners.end()) {
                return;
            }

            for (const auto ids = it->second; const EventListenerId listener_id: ids) {
                this->unlisten(listener_id);
            }
        }

        template<typename Event>
        void unlisten(const EventListenerId listener_id) {
            if (GlobalEventListener *listener = this->getListener(listener_id);
                listener == nullptr || listener->type != this->type<Event>()) {
                return;
            }
            this->unlisten(listener_id);
        }

        void unlistenAll(const ecs::Entity owner) {
            const auto it = this->owner_listeners.find(owner.index);
            if (it == this->owner_listeners.end()) {
                return;
            }

            for (const auto ids = it->second; const EventListenerId listener_id: ids) {
                this->unlisten(listener_id);
            }
        }

    private:
        template<typename Event, typename Func>
            requires std::invocable<Func, ecs::World &, const Event>
        EventListenerId add(const uint32_t owner, Func &&func) {
            using Callback = std::decay_t<Func>;

            const uint16_t event_type = this->type<Event>();
            const EventListenerId listener_id = this->listeners.size() + 1;

            auto &event_ids = this->event_listeners[event_type];
            const uint32_t event_slot = event_ids.size();
            event_ids.push_back(listener_id);

            uint32_t owner_slot = 0;
            if (owner != no_owner) {
                auto &owner_ids = this->owner_listeners[owner];
                owner_slot = owner_ids.size();
                owner_ids.push_back(listener_id);
            }

            this->listeners.push_back({
                true,
                event_type,
                owner,
                event_slot,
                owner_slot,
                new Callback(std::forward<Func>(func)),
                [](void *ptr, ecs::World &world, const void *event) {
                    (*static_cast<Callback *>(ptr))(world, *static_cast<const Event *>(event));
                },
                [](void *ptr) {
                    delete static_cast<Callback *>(ptr);
                }
            });

            return listener_id;
        }

        GlobalEventListener *getListener(const EventListenerId listener_id) {
            if (listener_id == 0 || listener_id > this->listeners.size()) {
                return nullptr;
            }

            auto &listener = this->listeners[listener_id - 1];
            if (!listener.alive) {
                return nullptr;
            }
            return &listener;
        }

        void unlisten(const EventListenerId listener_id) {
            GlobalEventListener *listener = this->getListener(listener_id);
            if (listener == nullptr) {
                return;
            }

            this->removeFromEvent(*listener, listener_id);
            if (listener->owner != no_owner) {
                this->removeFromOwner(*listener, listener_id);
            }

            if (listener->destroy) {
                listener->destroy(listener->callback);
            }
            *listener = {};
        }

        void removeFromEvent(const GlobalEventListener &listener, const EventListenerId listener_id) {
            const auto it = this->event_listeners.find(listener.type);
            if (it == this->event_listeners.end()) {
                return;
            }

            auto &ids = it->second;
            const EventListenerId moved = ids.back();
            ids[listener.event_slot] = moved;
            ids.pop_back();

            if (moved != listener_id) {
                this->listeners[moved - 1].event_slot = listener.event_slot;
            }
            if (ids.empty()) {
                this->event_listeners.erase(it);
            }
        }

        void removeFromOwner(const GlobalEventListener &listener, const EventListenerId listener_id) {
            const auto it = this->owner_listeners.find(listener.owner);
            if (it == this->owner_listeners.end()) {
                return;
            }

            auto &ids = it->second;
            const EventListenerId moved = ids.back();
            ids[listener.owner_slot] = moved;
            ids.pop_back();

            if (moved != listener_id) {
                this->listeners[moved - 1].owner_slot = listener.owner_slot;
            }
            if (ids.empty()) {
                this->owner_listeners.erase(it);
            }
        }
    };
}
