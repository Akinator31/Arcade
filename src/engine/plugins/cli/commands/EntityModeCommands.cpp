#include "EntityModeCommands.hpp"

#include "engine/parsing/JsonSerializer.hpp"

namespace cli::entity_mode_commands {
    CliPlugin::CommandHandler ls() {
        return [](CliPlugin &, const CliSession &session, ecs::World &world, Scanner &, std::ostream &output) {
            const ecs::internal::Archetype &table = world.archetype_registry.getArchetype(
                world.entity_registry.getRecord(session.inspected_entity).archetypeId);

            for (int i = 0; i < table.getType().count; i++) {
                if (const char *name = world.component_registry.getRecord(table.getType().data[i]).name) {
                    output << name << "\n";
                }
            }
        };
    }

    CliPlugin::CommandHandler children() {
        return [](CliPlugin &, const CliSession &session, ecs::World &world, Scanner &, std::ostream &output) {
            for (const auto &[parent, child]: world.iterRelated<Hierarchy>(session.inspected_entity)) {
                if (world.has<Name>(child)) {
                    output << world.get<Name>(child)->value << "\n";
                } else {
                    output << world.makeEntityName(child) << "\n";
                }
            }
        };
    }

    CliPlugin::CommandHandler print() {
        return [](CliPlugin &, const CliSession &session, ecs::World &world, Scanner &scanner, std::ostream &output) {
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

            const ecs::ComponentID cid = it->second;
            const ecs::internal::Archetype &table = world.archetype_registry.getArchetype(
                world.entity_registry.getRecord(session.inspected_entity).archetypeId);

            if (!table.has(cid)) {
                output << "Entity does not have component\n";
                return;
            }

            const auto &record = world.component_registry.getRecord(cid);
            if (!record.def) {
                output << "not reflectable\n";
                return;
            }
            if (record.size < record.def->size()) {
                output << "component storage does not match reflection\n";
                return;
            }

            const void *ptr = world.get_id(session.inspected_entity, cid);
            if (ptr == nullptr) {
                output << "component has no storage\n";
                return;
            }
            output << JsonSerializer::serialize(*record.def, ptr) << "\n";
        };
    }

    CliPlugin::CommandHandler set() {
        return [](CliPlugin &, const CliSession &session, ecs::World &world, Scanner &scanner, std::ostream &) {
            scanner.skip_whitespace();
            const std::string comp_name = scanner.take_identifier();
            if (comp_name.empty()) {
                return;
            }

            scanner.skip_whitespace();
            const std::string field_name = scanner.take_identifier();
            if (field_name.empty()) {
                return;
            }

            scanner.skip_whitespace();
            const std::string value = scanner.take_rest();
            if (value.empty()) {
                return;
            }

            const auto it = world.component_registry.name_to_id.find(comp_name);
            if (it == world.component_registry.name_to_id.end()) {
                return;
            }

            const ecs::ComponentID cid = it->second;
            const ecs::internal::Archetype &table = world.archetype_registry.getArchetype(
                world.entity_registry.getRecord(session.inspected_entity).archetypeId);

            if (!table.has(cid)) {
                return;
            }

            const auto &record = world.component_registry.getRecord(cid);
            if (!record.def) {
                return;
            }
            if (record.size < record.def->size()) {
                return;
            }

            if (cid == reflection::type_id<Name>() && field_name == "value") {
                world.set<Name>(session.inspected_entity, Name{value});
                return;
            }

            void *ptr = world.get_id(session.inspected_entity, cid);
            if (ptr == nullptr) {
                return;
            }
            record.def->from_string(ptr, field_name.c_str(), value);
            if (cid == reflection::type_id<Name>()) {
                world.syncEntityName(session.inspected_entity);
            }
        };
    }

    CliPlugin::CommandHandler add() {
        return [](CliPlugin &, const CliSession &session, ecs::World &world, Scanner &scanner, std::ostream &output) {
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

            world.add_id(session.inspected_entity, it->second);
        };
    }

    CliPlugin::CommandHandler remove() {
        return [](CliPlugin &, const CliSession &session, ecs::World &world, Scanner &scanner, std::ostream &output) {
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

            world.remove_id(session.inspected_entity, it->second);
        };
    }

    CliPlugin::CommandHandler exit() {
        return [](CliPlugin &, CliSession &session, ecs::World &, Scanner &, std::ostream &) {
            session.state = CliState::Normal;
        };
    }
}
