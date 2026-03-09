#pragma once

#include "Query.hpp"
#include <vector>
#include <tuple>

using SystemRegistered = std::pair<ecs::QueryID, void (*)(ArchetypeView &)>;

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
};

template<typename Phase>
struct On {
    using phase = Phase;
};

template<typename... Components>
struct Without {
    using without = All<Components...>;
};

template<float value>
struct Interval {
    static constexpr float interval = value;
};

template<typename System>
concept HasRequired = requires(System system, Query &query)
{
    {
        System::with::require(query)
    };
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
