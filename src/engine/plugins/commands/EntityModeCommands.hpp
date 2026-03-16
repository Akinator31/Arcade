#pragma once

#include "engine/plugins/CliPlugin.hpp"

namespace cli::entity_mode_commands {
    CliPlugin::CommandHandler ls();
    CliPlugin::CommandHandler children();
    CliPlugin::CommandHandler print();
    CliPlugin::CommandHandler set();
    CliPlugin::CommandHandler add();
    CliPlugin::CommandHandler remove();
    CliPlugin::CommandHandler exit();
}
