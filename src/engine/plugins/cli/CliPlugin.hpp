#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <iostream>
#include <ostream>
#include <string>
#include <unordered_map>

#include "engine/ecs/World.hpp"
#include "engine/network/TcpServer.hpp"
#include "engine/parsing/Scanner.hpp"

enum class CliState {
    Normal,
    Entity
};

enum class CliMode {
    Interactive,
    Server
};

struct CliSession {
    CliState state = CliState::Normal;
    ecs::Entity inspected_entity{};
};

struct CliPlugin;

SYSTEM(CliRuntimeSystem, On<PreUpdate>) {
    using CommandHandler = std::function<void(CliPlugin &, CliSession &, ecs::World &, Scanner &, std::ostream &)>;

    RUN(CliRuntimeSystem,, world);
};

struct CliPlugin {
    using CommandHandler = CliRuntimeSystem::CommandHandler;

    explicit CliPlugin(CliMode mode = CliMode::Interactive, uint16_t port = 0);

    CliMode mode;
    uint16_t port;
    CliSession session;
    std::unordered_map<int, CliSession> client_sessions;
    std::unordered_map<std::string, CommandHandler> normal_commands;
    std::unordered_map<std::string, CommandHandler> entity_commands;
    std::deque<std::string> stored_names;
    network::TcpServer server;

    void load(ecs::World &world);

    static void unload(ecs::World &world);

    void init_normal_commands();

    void init_entity_commands();

    [[nodiscard]] static std::string prompt(const CliSession &current_session);

    [[nodiscard]] std::string prompt() const;

    void execute_command(std::unordered_map<std::string, CommandHandler> &commands,
                         CliSession &current_session,
                         ecs::World &world,
                         Scanner &scanner,
                         std::ostream &output);

    void progress(ecs::World &world,
                  const std::string &input,
                  std::ostream &output,
                  CliSession &current_session);

    void progress(ecs::World &world, const std::string &input, std::ostream &output = std::cout);

    [[nodiscard]] std::string execute(ecs::World &world, const std::string &input, CliSession &current_session);

    [[nodiscard]] std::string execute(ecs::World &world, const std::string &input);

    void tick(ecs::World &world);

    static ecs::Entity create_named_entity(ecs::World &world, const std::string &name);

private:
    void start_server();

    void tick_server(ecs::World &world);

    void tick_interactive(ecs::World &world);

    void send_prompts_to_new_clients();

    void handle_client_messages(ecs::World &world);

    void remove_closed_sessions();

    const char *store_name(const std::string &name);
};
