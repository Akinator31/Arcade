#include "commands/EntityModeCommands.hpp"

void CliPlugin::init_entity_commands() {
    entity_commands["ls"] = cli::entity_mode_commands::ls();
    entity_commands["children"] = cli::entity_mode_commands::children();
    entity_commands["print"] = cli::entity_mode_commands::print();
    entity_commands["set"] = cli::entity_mode_commands::set();
    entity_commands["add"] = cli::entity_mode_commands::add();
    entity_commands["remove"] = cli::entity_mode_commands::remove();
    entity_commands["exit"] = cli::entity_mode_commands::exit();
}
