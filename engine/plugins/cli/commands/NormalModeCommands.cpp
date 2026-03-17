#include "NormalModeCommands.hpp"

#include "engine/parsing/JsonSerializer.hpp"

namespace cli::normal_mode_commands {
    CliPlugin::CommandHandler ls() {
        return [](CliPlugin &, CliSession &, const ecs::World &world, Scanner &, std::ostream &output) {
            for (const auto &comp: world.component_registry.components) {
                if (comp.name) {
                    output << comp.name << "\n";
                }
            }
        };
    }

    CliPlugin::CommandHandler entities() {
        return [](CliPlugin &, CliSession &, ecs::World &world, Scanner &, std::ostream &output) {
            world.fetch<Name>().iter([&output](ArchetypeView &view) {
                const auto *names = view.column<Name>();
                for (uint i = 0; i < view.count(); i++) {
                    output << names[i].value << "\n";
                }
            });
        };
    }

    CliPlugin::CommandHandler roots() {
        return [](CliPlugin &, CliSession &, ecs::World &world, Scanner &, std::ostream &output) {
            world.fetch<>().iter([&](ArchetypeView &view) {
                const auto *names = view.optional<Name>();
                for (uint i = 0; i < view.count(); i++) {
                    const ecs::Entity entity = view.entity(i);
                    if (world.has<Parent>(entity)) {
                        continue;
                    }
                    if (names != nullptr) {
                        output << names[i].value << "\n";
                    } else {
                        output << ecs::World::makeEntityName(entity) << "\n";
                    }
                }
            });
        };
    }

    CliPlugin::CommandHandler create() {
        return [](CliPlugin &, CliSession &, ecs::World &world, Scanner &scanner, std::ostream &) {
            scanner.skip_whitespace();
            const std::string name = scanner.take_rest();
            if (name.empty()) {
                return;
            }
            CliPlugin::create_named_entity(world, name);
        };
    }

    CliPlugin::CommandHandler remove() {
        return [](CliPlugin &, CliSession &, ecs::World &world, Scanner &scanner, std::ostream &) {
            scanner.skip_whitespace();
            const std::string name = scanner.take_rest();
            if (name.empty()) {
                return;
            }
            if (const auto entity = world.findEntityByName(name); entity.has_value()) {
                world.kill(entity.value());
            }
        };
    }

    CliPlugin::CommandHandler print() {
        return [](CliPlugin &, CliSession &, ecs::World &world, Scanner &scanner, std::ostream &output) {
            scanner.skip_whitespace();
            const std::string name = scanner.take_identifier();
            if (name.empty()) {
                return;
            }

            const auto it = world.component_registry.name_to_id.find(name);
            if (it == world.component_registry.name_to_id.end()) {
                output << "Component not found\n";
                return;
            }

            const auto &record = world.component_registry.getRecord(it->second);
            if (!record.def) {
                output << "not reflectable\n";
                return;
            }
            if (record.size < record.def->size()) {
                output << "component storage does not match reflection\n";
                return;
            }

            output << JsonSerializer::serialize_default(
                *record.def,
                record.size,
                [&](void *ptr) {
                    if (record.construct != nullptr) {
                        record.construct(world, ptr);
                    }
                }
            ) << "\n";
        };
    }

    CliPlugin::CommandHandler inspect() {
        return [](CliPlugin &, CliSession &session, const ecs::World &world, Scanner &scanner, std::ostream &) {
            scanner.skip_whitespace();
            const std::string name = scanner.take_rest();
            if (name.empty()) {
                return;
            }
            if (const auto entity = world.findEntityByName(name); entity.has_value()) {
                session.inspected_entity = entity.value();
                session.state = CliState::Entity;
            }
        };
    }

    CliPlugin::CommandHandler progress() {
        return [](CliPlugin &, CliSession &, ecs::World &world, Scanner &, std::ostream &) {
            world.progress();
        };
    }
}
