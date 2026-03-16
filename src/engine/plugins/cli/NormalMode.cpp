#include "commands/NormalModeCommands.hpp"

void CliPlugin::init_normal_commands() {
    normal_commands["ls"] = cli::normal_mode_commands::ls();
    normal_commands["entities"] = cli::normal_mode_commands::entities();
    normal_commands["roots"] = cli::normal_mode_commands::roots();
    normal_commands["create"] = cli::normal_mode_commands::create();
    normal_commands["delete"] = cli::normal_mode_commands::remove();
    normal_commands["inspect"] = cli::normal_mode_commands::inspect();
    normal_commands["progress"] = cli::normal_mode_commands::progress();
}
