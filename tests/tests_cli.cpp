#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <arpa/inet.h>
#include <chrono>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include "engine/ecs/World.hpp"
#include "engine/plugins/CliPlugin.hpp"

struct Position {
    float x, y;

    rayflect(Position,
             Position->member<float>("x");
             Position->member<float>("y");
    )
};

struct Enemy;

void redirect_all_stdout() {
    cr_redirect_stdout();
    cr_redirect_stderr();
}

uint16_t find_free_port() {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    cr_assert_neq(fd, -1);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    cr_assert_eq(bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)), 0);

    socklen_t len = sizeof(addr);
    cr_assert_eq(getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len), 0);

    const uint16_t port = ntohs(addr.sin_port);
    close(fd);
    return port;
}

int connect_to_server(const uint16_t port) {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    cr_assert_neq(fd, -1);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    cr_assert_eq(connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)), 0);
    return fd;
}

std::string read_from_socket(const int fd) {
    char buffer[4096];
    const ssize_t size = recv(fd, buffer, sizeof(buffer), MSG_DONTWAIT);
    if (size <= 0) {
        return "";
    }
    return {buffer, static_cast<std::size_t>(size)};
}

std::string wait_for_socket_data(ecs::World& world, const int fd, const int max_ticks = 50) {
    for (int i = 0; i < max_ticks; i++) {
        world.progress();
        if (const std::string data = read_from_socket(fd); !data.empty()) {
            return data;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return "";
}

Test(cli, normal_ls, .init = redirect_all_stdout) {
    ecs::World world;
    CliPlugin cli;

    world.component_registry.registerComponent<Position>();

    cli.progress(world, "ls");

    fflush(stdout);
    cr_assert_stdout_neq_str("");
}

Test(cli, normal_entities, .init = redirect_all_stdout) {
    ecs::World world;
    CliPlugin cli;

    const ecs::Entity e1 = world.entity();
    world.add<Name>(e1);
    world.get<Name>(e1)->value = "Player1";

    cli.progress(world, "entities");

    fflush(stdout);
    cr_assert_stdout_eq_str("Player1\n");
}

Test(cli, normal_create, .init = redirect_all_stdout) {
    ecs::World world;
    CliPlugin cli;

    cli.progress(world, "create Player1");
    cli.progress(world, "entities");

    fflush(stdout);
    cr_assert_stdout_eq_str("Player1\n");
}

Test(cli, normal_create_then_inspect, .init = redirect_all_stdout) {
    ecs::World world;
    CliPlugin cli;

    cli.progress(world, "create Player1");
    cli.progress(world, "inspect Player1");

    cr_assert(cli.session.state == CliState::Entity);
}

Test(cli, normal_delete, .init = redirect_all_stdout) {
    ecs::World world;
    CliPlugin cli;

    const ecs::Entity e1 = world.entity();
    world.add<Name>(e1);
    world.get<Name>(e1)->value = "Player1";

    cli.progress(world, "delete Player1");
    cli.progress(world, "entities");

    fflush(stdout);
    cr_assert_stdout_eq_str("");
    cr_assert(!world.isAlive(e1));
}

Test(cli, normal_inspect_success, .init = redirect_all_stdout) {
    ecs::World world;
    CliPlugin cli;

    const ecs::Entity e1 = world.entity();
    world.add<Name>(e1);
    world.get<Name>(e1)->value = "Player1";

    cr_assert(cli.session.state == CliState::Normal);
    cli.progress(world, "inspect Player1");
    cr_assert(cli.session.state == CliState::Entity);
    cr_assert_eq(cli.session.inspected_entity.index, e1.index);
    cr_assert_eq(cli.session.inspected_entity.generation, e1.generation);
}

Test(cli, normal_inspect_fail, .init = redirect_all_stdout) {
    ecs::World world;
    CliPlugin cli;

    const ecs::Entity e1 = world.entity();
    world.add<Name>(e1);
    world.get<Name>(e1)->value = "Player1";

    cr_assert(cli.session.state == CliState::Normal);
    cli.progress(world, "inspect Player2");
    cr_assert(cli.session.state == CliState::Normal);
}

Test(cli, entity_exit, .init = redirect_all_stdout) {
    ecs::World world;
    CliPlugin cli;

    cli.session.state = CliState::Entity;
    cli.progress(world, "exit");
    cr_assert(cli.session.state == CliState::Normal);
}

Test(cli, entity_ls, .init = redirect_all_stdout) {
    ecs::World world;
    CliPlugin cli;

    world.component_registry.registerComponent<Position>();
    world.component_registry.registerComponent<Name>();

    const ecs::Entity e1 = world.entity();
    world.add<Name>(e1);
    world.get<Name>(e1)->value = "Player1";
    world.add<Position>(e1);

    cli.session.state = CliState::Normal;
    cli.progress(world, "inspect Player1");
    cr_assert(cli.session.state == CliState::Entity);

    cli.progress(world, "ls");

    fflush(stdout);
    cr_assert_stdout_neq_str("");
}

Test(cli, entity_print, .init = redirect_all_stdout) {
    ecs::World world;
    CliPlugin cli;

    world.component_registry.registerComponent<Position>();
    world.component_registry.registerComponent<Name>();

    ecs::Entity e1 = world.entity();
    world.add<Name>(e1);
    world.get<Name>(e1)->value = "Player1";
    world.add<Position>(e1);
    world.get<Position>(e1)->x = 10.0f;
    world.get<Position>(e1)->y = 20.0f;

    cli.session.state = CliState::Normal;
    cli.progress(world, "inspect Player1");

    cli.progress(world, "print Position");

    fflush(stdout);
    const char* expected_pos = "{\n"
                               "  \"x\": 10,\n"
                               "  \"y\": 20\n"
                               "}\n";
    cr_assert_stdout_eq_str(expected_pos);
}

Test(cli, entity_set, .init = redirect_all_stdout) {
    ecs::World world;
    CliPlugin cli;

    world.component_registry.registerComponent<Position>();
    world.component_registry.registerComponent<Name>();

    ecs::Entity e1 = world.entity();
    world.add<Name>(e1);
    world.get<Name>(e1)->value = "Player1";
    world.add<Position>(e1);
    world.get<Position>(e1)->x = 10.0f;
    world.get<Position>(e1)->y = 20.0f;

    cli.session.state = CliState::Normal;
    cli.progress(world, "inspect Player1");

    cli.progress(world, "set Position x 42.5");
    cli.progress(world, "set Position y -15.0");
    cli.progress(world, "set Name value Hero");

    cr_assert_eq(world.get<Position>(e1)->x, 42.5f);
    cr_assert_eq(world.get<Position>(e1)->y, -15.0f);
    cr_assert_str_eq(world.get<Name>(e1)->value, "Hero");
}

Test(cli, entity_add, .init = redirect_all_stdout) {
    ecs::World world;
    CliPlugin cli;

    world.component_registry.registerComponent<Position>();
    world.component_registry.registerComponent<Name>();

    const ecs::Entity e1 = world.entity();
    world.add<Name>(e1);
    world.get<Name>(e1)->value = "Player1";

    cli.session.state = CliState::Normal;
    cli.progress(world, "inspect Player1");
    cli.progress(world, "add Position");

    cr_assert_not_null(world.get<Position>(e1));
}

Test(cli, entity_add_missing_component, .init = redirect_all_stdout) {
    ecs::World world;
    CliPlugin cli;

    world.component_registry.registerComponent<Name>();

    const ecs::Entity e1 = world.entity();
    world.add<Name>(e1);
    world.get<Name>(e1)->value = "Player1";

    cli.session.state = CliState::Normal;
    cli.progress(world, "inspect Player1");
    cli.progress(world, "add Position");

    fflush(stdout);
    cr_assert_stdout_eq_str("Component not found\n");
}

Test(cli, entity_remove, .init = redirect_all_stdout) {
    ecs::World world;
    CliPlugin cli;

    world.component_registry.registerComponent<Position>();
    world.component_registry.registerComponent<Name>();

    const ecs::Entity e1 = world.entity();
    world.add<Name>(e1);
    world.get<Name>(e1)->value = "Player1";
    world.add<Position>(e1);

    cli.session.state = CliState::Normal;
    cli.progress(world, "inspect Player1");
    cli.progress(world, "remove Position");

    cr_assert_null(world.get<Position>(e1));
}

Test(cli, entity_remove_missing_component, .init = redirect_all_stdout) {
    ecs::World world;
    CliPlugin cli;

    world.component_registry.registerComponent<Name>();

    const ecs::Entity e1 = world.entity();
    world.add<Name>(e1);
    world.get<Name>(e1)->value = "Player1";

    cli.session.state = CliState::Normal;
    cli.progress(world, "inspect Player1");
    cli.progress(world, "remove Position");

    fflush(stdout);
    cr_assert_stdout_eq_str("Component not found\n");
}

Test(cli, entity_remove_incomplete_tag, .init = redirect_all_stdout) {
    ecs::World world;
    CliPlugin cli;

    world.component_registry.registerComponent<Name>();
    world.component_registry.registerComponent<Enemy>();

    const ecs::Entity e1 = world.entity();
    world.add<Name>(e1);
    world.get<Name>(e1)->value = "Player1";
    world.add<Enemy>(e1);

    cr_assert(world.has<Enemy>(e1));

    cli.session.state = CliState::Normal;
    cli.progress(world, "inspect Player1");
    cli.progress(world, "remove Enemy");

    cr_assert(!world.has<Enemy>(e1));
}

Test(cli, entity_print_name, .init = redirect_all_stdout) {
    ecs::World world;
    CliPlugin cli;

    world.component_registry.registerComponent<Name>();

    const ecs::Entity e1 = world.entity();
    world.add<Name>(e1);
    world.get<Name>(e1)->value = "Hero";

    cli.session.state = CliState::Normal;
    cli.progress(world, "inspect Hero");

    cli.progress(world, "print Name");

    fflush(stdout);
    const auto expected_name = "{\n"
            "  \"value\": \"Hero\"\n"
            "}\n";
    cr_assert_stdout_eq_str(expected_name);
}

Test(cli, empty_command) {
    ecs::World world;
    CliPlugin cli;

    cli.progress(world, "");
    cli.progress(world, "    ");
}

Test(cli, prompt_changes_with_state) {
    CliPlugin cli;

    const std::string normal_prompt = cli.prompt();
    cr_assert_str_eq(normal_prompt.c_str(), "> ");

    cli.session.state = CliState::Entity;
    cli.session.inspected_entity = {42, 7};

    const std::string entity_prompt = cli.prompt();
    cr_assert_str_eq(entity_prompt.c_str(), "entity(42, 7)> ");
}

Test(cli, server_execute_returns_output_and_prompt) {
    ecs::World world;
    CliPlugin cli(CliMode::Server, 4242);

    world.component_registry.registerComponent<Name>();

    const ecs::Entity e1 = world.entity();
    world.add<Name>(e1);
    world.get<Name>(e1)->value = "Hero";

    const std::string entities = cli.execute(world, "entities");
    cr_assert_str_eq(entities.c_str(), "Hero\n> ");

    const std::string inspect = cli.execute(world, "inspect Hero");
    const std::string expected = "entity(" + std::to_string(e1.index) + ", " + std::to_string(e1.generation) + ")> ";
    cr_assert_str_eq(inspect.c_str(), expected.c_str());
}

struct CliServerTickCounter {
    inline static int value = 0;
};

SYSTEM(CliServerTickSystem, On<Update>) {
    RUN(CliServerTickSystem,, world) {
        (void) world;
        CliServerTickCounter::value++;
    }
};

Test(cli, server_mode_does_not_block_world_progress) {
    ecs::World world;
    CliServerTickCounter::value = 0;

    world.plugin<CliPlugin>(CliMode::Server, find_free_port());
    world.system<CliServerTickSystem>();

    world.progress();
    world.progress();
    world.progress();

    cr_assert_eq(CliServerTickCounter::value, 3);
}

Test(cli, server_mode_serves_prompt_and_commands) {
    ecs::World world;
    const uint16_t port = find_free_port();

    world.component_registry.registerComponent<Name>();

    const ecs::Entity e1 = world.entity();
    world.add<Name>(e1);
    world.get<Name>(e1)->value = "Hero";

    world.plugin<CliPlugin>(CliMode::Server, port);

    const int client_fd = connect_to_server(port);

    const std::string prompt = wait_for_socket_data(world, client_fd);
    cr_assert_str_eq(prompt.c_str(), "> ");

    const std::string command = "inspect Hero\n";
    cr_assert_eq(send(client_fd, command.c_str(), command.size(), 0), static_cast<ssize_t>(command.size()));

    const std::string response = wait_for_socket_data(world, client_fd);
    const std::string expected = "entity(" + std::to_string(e1.index) + ", " + std::to_string(e1.generation) + ")> ";
    cr_assert_str_eq(response.c_str(), expected.c_str());

    close(client_fd);
}
