#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace network {
    struct TcpClient {
        int fd = -1;
        std::string buffer;
        bool disconnected = false;
        bool just_connected = false;
    };

    class TcpServer {
        int listen_fd = -1;
        std::vector<TcpClient> clients;

    public:
        TcpServer() = default;
        ~TcpServer();

        TcpServer(const TcpServer&) = delete;
        TcpServer& operator=(const TcpServer&) = delete;
        TcpServer(TcpServer&& other) noexcept;
        TcpServer& operator=(TcpServer&& other) noexcept;

        bool start(uint16_t port);
        void poll_events();

        std::vector<std::pair<int, std::string>> get_messages();
        std::vector<int> get_new_connections();

        static void send_to(int fd, const std::string& msg);
        void disconnect(int fd);

        [[nodiscard]] const std::vector<TcpClient>& get_clients() const { return clients; }
    };
}
