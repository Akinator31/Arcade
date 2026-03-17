#pragma once

#include "engine/plugins/cli/CliPlugin.hpp"

namespace cli::normal_mode_commands {
    CliPlugin::CommandHandler ls();

    CliPlugin::CommandHandler entities();

    CliPlugin::CommandHandler roots();

    CliPlugin::CommandHandler create();

    CliPlugin::CommandHandler remove();

    CliPlugin::CommandHandler print();

    CliPlugin::CommandHandler inspect();

    CliPlugin::CommandHandler progress();
}
