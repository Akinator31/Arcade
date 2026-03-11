#include "CliPlugin.hpp"
#include <cstring>

void CliPlugin::init_normal_commands() {
    normal_commands["ls"] = [](CliPlugin&, CliSession&, ecs::World& world, Scanner&, std::ostream& output) {
        for (const auto& comp : world.component_registry.components) {
            if (comp.name) {
                output << comp.name << "\n";
            }
        }
    };

    normal_commands["entities"] = [](CliPlugin&, CliSession&, ecs::World& world, Scanner&, std::ostream& output) {
        world.fetch<Name>().iter([&output](ArchetypeView& view) {
            const auto* names = view.column<Name>();
            for (uint i = 0; i < view.count(); i++) {
                output << names[i].value << "\n";
            }
        });
    };

    normal_commands["inspect"] = [](CliPlugin&, CliSession& session, ecs::World& world, Scanner& scanner, std::ostream&) {
        scanner.skip_whitespace();

        const std::string name = scanner.take_identifier();
        if (name.empty()) {
            return;
        }

        world.fetch<Name>().iter([&](ArchetypeView& view) {
            const auto* names = view.column<Name>();
            for (uint i = 0; i < view.count(); i++) {
                if (std::strcmp(names[i].value, name.c_str()) == 0) {
                    session.inspected_entity = view.entity(i);
                    session.state = CliState::Entity;
                    return;
                }
            }
        });
    };
}
