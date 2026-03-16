#include "NormalModeCommands.hpp"

namespace cli::normal_mode_commands {
    CliPlugin::CommandHandler ls() {
        return [](CliPlugin &, CliSession &, ecs::World &world, Scanner &, std::ostream &output) {
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
            world.fetch<Name>().iter([&](ArchetypeView &view) {
                const auto *names = view.column<Name>();
                for (uint i = 0; i < view.count(); i++) {
                    const ecs::Entity entity = view.entity(i);
                    if (world.has<Parent>(entity)) {
                        continue;
                    }
                    output << names[i].value << "\n";
                }
            });
        };
    }

    CliPlugin::CommandHandler create() {
        return [](CliPlugin &cli, CliSession &, ecs::World &world, Scanner &scanner, std::ostream &) {
            scanner.skip_whitespace();
            const std::string name = scanner.take_rest();
            if (name.empty()) {
                return;
            }
            cli.create_named_entity(world, name);
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

    CliPlugin::CommandHandler inspect() {
        return [](CliPlugin &, CliSession &session, ecs::World &world, Scanner &scanner, std::ostream &) {
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
}
