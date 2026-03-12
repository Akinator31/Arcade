#include "CliPlugin.hpp"
#include "engine/parsing/JsonSerializer.hpp"

void CliPlugin::init_entity_commands() {
    entity_commands["ls"] = [](CliPlugin &, const CliSession &session, ecs::World &world, Scanner &,
                               std::ostream &output) {
        const ecs::internal::Archetype &table = world.archetype_registry.getArchetype(
            world.entity_registry.getRecord(session.inspected_entity).archetypeId);

        for (int i = 0; i < table.getType().count; i++) {
            if (const char *name = world.component_registry.getRecord(table.getType().data[i]).name) {
                output << name << "\n";
            }
        }
    };

    entity_commands["print"] = [](CliPlugin &, const CliSession &session, ecs::World &world, Scanner &scanner,
                                  std::ostream &output) {
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

        const void *ptr = world.get_id(session.inspected_entity, cid);
        output << JsonSerializer::serialize(*record.def, ptr) << "\n";
    };

    entity_commands["set"] = [](CliPlugin &, CliSession &session, ecs::World &world, Scanner &scanner, std::ostream &) {
        scanner.skip_whitespace();
        const std::string comp_name = scanner.take_identifier();
        if (comp_name.empty()) return;

        scanner.skip_whitespace();
        const std::string field_name = scanner.take_identifier();
        if (field_name.empty()) return;

        scanner.skip_whitespace();
        const std::string value = scanner.take_rest();
        if (value.empty()) return;

        const auto it = world.component_registry.name_to_id.find(comp_name);
        if (it == world.component_registry.name_to_id.end()) return;

        const ecs::ComponentID cid = it->second;
        const ecs::internal::Archetype &table = world.archetype_registry.getArchetype(
            world.entity_registry.getRecord(session.inspected_entity).archetypeId);

        if (!table.has(cid)) return;

        const auto &record = world.component_registry.getRecord(cid);
        if (!record.def) return;

        void *ptr = world.get_id(session.inspected_entity, cid);
        record.def->from_string(ptr, field_name.c_str(), value);
    };

    entity_commands["add"] = [](CliPlugin &, const CliSession &session, ecs::World &world, Scanner &scanner,
                                std::ostream &output) {
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

    entity_commands["remove"] = [](CliPlugin &, const CliSession &session, ecs::World &world, Scanner &scanner,
                                   std::ostream &output) {
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

    entity_commands["exit"] = [](CliPlugin &, CliSession &session, ecs::World &, Scanner &, std::ostream &) {
        session.state = CliState::Normal;
    };
}
