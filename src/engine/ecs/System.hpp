#pragma once

#include "Query.hpp"
#include <vector>
#include <tuple>

#include "engine/reflection/TypeCounter.hpp"

namespace ecs {
    class World;
}

struct System {
};

using SystemCounter = reflection::TypeCounter<System>;

struct SystemRegistered {
    uint32_t id = 0;

    ecs::QueryID qid = 0;

    void (*iter)(ArchetypeView &) = nullptr;

    void *value = nullptr;

    void (*run)(void *, ecs::World &) = nullptr;

    void (*destroy)(void *) = nullptr;

    bool (*condition)(ecs::World &) = nullptr;
};

using ObserverFunc = void(*)(ecs::internal::Archetype &, ecs::internal::EntityRow row);

struct Phase {
    std::vector<SystemRegistered> systems;
};

template<typename... Components>
struct All {
    static void require(Query &query) {
        query.required<Components...>();
    }

    static void exclude(Query &query) {
        query.excluded<Components...>();
    }
};

template<typename System>
concept IsSystem = requires(ArchetypeView &view)
{
    {
        System::iter(view)
    };
};

template<typename... Components>
struct With {
    using with = All<Components...>;

    static void add_removed_components(ecs::internal::Archetype &arch, ObserverFunc func) {
        (arch.columns.get(reflection::type_id<Components>()).onRemove.push_back(func), ...);
    }

    static void remove_removed_components(ecs::internal::Archetype &arch, ObserverFunc func) {
        (arch.columns.get(reflection::type_id<Components>()).onRemove.remove(func), ...);
    }
};

struct Add {
};

struct Remove {
};


struct Despawn {
};


template<typename T>
concept IsObserver = requires(ecs::internal::Archetype &table, ecs::internal::EntityRow row)
{
    { T::observe(table, row) };
};


template<typename Phase>
struct On {
    using phase = Phase;
};

template<typename... Components>
struct Without {
    using without = All<Components...>;
};


template<typename System>
concept HasRequired = requires(System system, Query &query)
{
    {
        System::with::require(query)
    };
};

template<typename System>
concept Runnable = requires(ecs::World &world, System *sys)
{
    {
        System::run(sys, world)
    };
};

template<typename System>
concept IsSystemCondition = requires(ecs::World &world)
{
    { System::condition(world) } -> std::same_as<bool>;
};

template<IsSystemCondition ...Condition>
struct Conditions {
    static bool condition(ecs::World &world) {
        return (Condition::condition(world) && ...);
    }
};

template<class System>
constexpr auto getSystemCondition() {
    if constexpr (IsSystemCondition<System>)
        return System::condition;
    else
        return nullptr;
}


template<typename... Components>
struct SystemParams : Components... {
};

template<typename System>
concept HasPhase = requires()
{
    typename System::phase;
};


template<typename System>
concept HasExcluded = requires(System system, Query &query)
{
    {
        System::without::exclude(query)
    };
};

#define ITER(view) static void iter(ArchetypeView &view)
#define RUN(type, self, world) static void run(type *self, ecs::World &world)
#define OBSERVE(table, row) static void observe(ecs::internal::Archetype &table, ecs::internal::EntityRow row)
#define SYSTEM(name, ...) struct name : __VA_ARGS__

#if defined(_MSC_VER)
#define RESTRICT __restrict
#elif defined(__GNUC__) || defined(__clang__)
#define RESTRICT __restrict__
#else
#define RESTRICT
#endif

struct PreStartup {
};

struct Startup {
};

struct PostStartup {
};

struct PreUpdate {
};

struct Update {
};

struct PostUpdate {
};

struct PreRender {
};

struct Render {
};

using PhaseId = std::size_t;

using SystemId = std::tuple<PhaseId, uint32_t>;

struct PhaseContainer {
    std::vector<int> phases;

    template<typename Phase>
    PhaseId phase() {
        static PhaseId id = [](PhaseContainer *self) {
            self->phases.emplace_back();
            return self->phases.size() - 1;
        }(this);

        return id;
    }
};
