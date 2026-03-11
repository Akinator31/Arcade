#include "TcpServer.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <algorithm>
#include <iostream>

namespace network {
    TcpServer::~TcpServer() {
        if (listen_fd != -1) {
            close(listen_fd);
        }
        for (const auto &client: clients) {
            close(client.fd);
        }
    }

    TcpServer::TcpServer(TcpServer &&other) noexcept : listen_fd(other.listen_fd), clients(std::move(other.clients)) {
        other.listen_fd = -1;
    }

    TcpServer &TcpServer::operator=(TcpServer &&other) noexcept {
        if (this != &other) {
            if (listen_fd != -1) close(listen_fd);
            for (const auto &client: clients) close(client.fd);

            listen_fd = other.listen_fd;
            clients = std::move(other.clients);

            other.listen_fd = -1;
        }
        return *this;
    }

    bool TcpServer::start(const uint16_t port) {
        listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd < 0) return false;

        int opt = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        int flags = fcntl(listen_fd, F_GETFL, 0);
        fcntl(listen_fd, F_SETFL, flags | O_NONBLOCK);

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(listen_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) return false;
        if (listen(listen_fd, 10) < 0) return false;

        return true;
    }

    void TcpServer::poll_events() {
        if (listen_fd == -1) return;

        std::vector<pollfd> fds;
        fds.push_back({listen_fd, POLLIN, 0});
        for (const auto &client: clients) {
            fds.push_back({client.fd, POLLIN, 0});
        }

        if (poll(fds.data(), fds.size(), 0) <= 0) return;

        if (fds[0].revents & POLLIN) {
            if (const int client_fd = accept(listen_fd, nullptr, nullptr); client_fd >= 0) {
                const int flags = fcntl(client_fd, F_GETFL, 0);
                fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
                clients.push_back({client_fd, "", false, true});
            }
        }

        for (size_t i = 1; i < fds.size(); ++i) {
            if (fds[i].revents & POLLIN) {
                char buf[1024];
                if (const ssize_t n = recv(fds[i].fd, buf, sizeof(buf), 0); n <= 0) {
                    clients[i - 1].disconnected = true;
                } else {
                    clients[i - 1].buffer.append(buf, n);
                }
            } else if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                clients[i - 1].disconnected = true;
            }
        }

        const auto it = std::ranges::remove_if(clients, [](const TcpClient &c) {
            if (c.disconnected) {
                close(c.fd);
                return true;
            }
            return false;
        }).begin();
        clients.erase(it, clients.end());
    }

    std::vector<std::pair<int, std::string> > TcpServer::get_messages() {
        std::vector<std::pair<int, std::string> > messages;
        for (auto &client: clients) {
            size_t pos;
            while ((pos = client.buffer.find('\n')) != std::string::npos) {
                std::string msg = client.buffer.substr(0, pos);
                if (!msg.empty() && msg.back() == '\r') {
                    msg.pop_back();
                }
                messages.emplace_back(client.fd, msg);
                client.buffer.erase(0, pos + 1);
            }
        }
        return messages;
    }

    std::vector<int> TcpServer::get_new_connections() {
        std::vector<int> new_connections;
        for (auto &client: clients) {
            if (client.just_connected) {
                new_connections.push_back(client.fd);
                client.just_connected = false;
            }
        }
        return new_connections;
    }

    void TcpServer::send_to(const int fd, const std::string &msg) {
        send(fd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
    }

    void TcpServer::disconnect(const int fd) {
        for (auto &client: clients) {
            if (client.fd == fd) {
                client.disconnected = true;
            }
        }
    }
}
