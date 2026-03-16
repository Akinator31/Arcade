#pragma once

#include "engine/plugins/CliPlugin.hpp"

namespace cli::normal_mode_commands {
    CliPlugin::CommandHandler ls();
    CliPlugin::CommandHandler entities();
    CliPlugin::CommandHandler roots();
    CliPlugin::CommandHandler create();
    CliPlugin::CommandHandler remove();
    CliPlugin::CommandHandler inspect();
}
