#include "CliPlugin.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

CliPlugin::CliPlugin(const CliMode mode, const uint16_t port) : mode(mode), port(port) {
    init_normal_commands();
    init_entity_commands();
}

void CliPlugin::load(ecs::World &world) {
    world.registerComponent<Name>();
    if (mode == CliMode::Server) {
        start_server();
    }
    world.system<CliRuntimeSystem>();
}

void CliPlugin::unload(ecs::World &) {
}

std::string CliPlugin::prompt(const CliSession &current_session) {
    if (current_session.state == CliState::Entity) {
        std::ostringstream oss;
        oss << "entity(" << current_session.inspected_entity.index << ", "
                << current_session.inspected_entity.generation << ")> ";
        return oss.str();
    }
    return "> ";
}

std::string CliPlugin::prompt() const {
    return prompt(session);
}

void CliPlugin::execute_command(std::unordered_map<std::string, CommandHandler> &commands,
                                CliSession &current_session,
                                ecs::World &world,
                                Scanner &scanner,
                                std::ostream &output) {
    const std::string command_name = scanner.take_identifier();
    if (command_name.empty()) {
        return;
    }

    if (const auto it = commands.find(command_name); it != commands.end()) {
        it->second(*this, current_session, world, scanner, output);
    }
}

void CliPlugin::progress(ecs::World &world,
                         const std::string &input,
                         std::ostream &output,
                         CliSession &current_session) {
    Scanner scanner(input.c_str());

    if (current_session.state == CliState::Normal) {
        execute_command(normal_commands, current_session, world, scanner, output);
        return;
    }

    execute_command(entity_commands, current_session, world, scanner, output);
}

void CliPlugin::progress(ecs::World &world, const std::string &input, std::ostream &output) {
    progress(world, input, output, session);
}

std::string CliPlugin::execute(ecs::World &world, const std::string &input, CliSession &current_session) {
    std::ostringstream output;
    progress(world, input, output, current_session);
    output << prompt(current_session);
    return output.str();
}

std::string CliPlugin::execute(ecs::World &world, const std::string &input) {
    return execute(world, input, session);
}

ecs::Entity CliPlugin::create_named_entity(ecs::World &world, const std::string &name) {
    const ecs::Entity entity = world.entity();
    world.set<Name>(entity, Name{name});
    return entity;
}

void CliPlugin::tick(ecs::World &world) {
    if (mode == CliMode::Server) {
        tick_server(world);
        return;
    }

    tick_interactive(world);
}

void CliPlugin::start_server() {
    if (port == 0) {
        throw std::invalid_argument("CliPlugin server mode requires a port");
    }
    if (!server.start(port)) {
        throw std::runtime_error("CliPlugin failed to start server");
    }
}

void CliPlugin::tick_server(ecs::World &world) {
    server.poll_events();
    send_prompts_to_new_clients();
    handle_client_messages(world);
    remove_closed_sessions();
}

void CliPlugin::tick_interactive(ecs::World &world) {
    std::cout << prompt();

    std::string input;
    if (!std::getline(std::cin, input) || input.empty()) {
        return;
    }

    progress(world, input);
}

void CliPlugin::send_prompts_to_new_clients() {
    for (const int fd: server.get_new_connections()) {
        client_sessions.try_emplace(fd);
        network::TcpServer::send_to(fd, prompt(client_sessions.at(fd)));
    }
}

void CliPlugin::handle_client_messages(ecs::World &world) {
    for (const auto &[fd, input]: server.get_messages()) {
        CliSession &current_session = client_sessions.try_emplace(fd).first->second;
        network::TcpServer::send_to(fd, execute(world, input, current_session));
    }
}

void CliPlugin::remove_closed_sessions() {
    std::erase_if(client_sessions, [this](const auto &entry) {
        return std::ranges::find_if(
                   server.get_clients(),
                   [&](const network::TcpClient &client) {
                       return client.fd == entry.first;
                   }
               ) == server.get_clients().end();
    });
}

const char *CliPlugin::store_name(const std::string &name) {
    stored_names.push_back(name);
    return stored_names.back().c_str();
}

void CliRuntimeSystem::run(CliRuntimeSystem *, ecs::World &world) {
    if (auto *cli = world.getPlugin<CliPlugin>()) {
        cli->tick(world);
    }
}
