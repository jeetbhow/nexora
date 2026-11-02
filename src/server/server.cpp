#include "nexora/server/server.hpp"

#include <memory>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

namespace nexora {
    constexpr int backlog = 10;

    void Server::close_with_error(int& fd, const char*& error, const char* message) {
        close(fd);
        error = message;
        fd = -1;
    }

    Server::~Server() {
        if (fd_ >= 0) {
            close(fd_);
        }
    }

    void Server::listen(int port) {
        if (fd_ >= 0) {
            throw runtime_error("server socket is already bound");
        }

        fd_ = create_listener(port);
    }

    int Server::create_listener(int port) {
        if (port < 0 || port > 65535) {
            throw runtime_error("server port must be between 0 and 65535");
        }

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        addrinfo* addresses = nullptr;
        string service = to_string(port);
        
        int status = getaddrinfo("127.0.0.1", service.c_str(), &hints, &addresses);
        if (status != 0) {
            throw runtime_error(string("failed to resolve server address: ") + gai_strerror(status));
        }

        unique_ptr<addrinfo, decltype(&freeaddrinfo)> address_list(addresses, &freeaddrinfo);

        const char* error = "failed to bind server socket";
        for (auto* addr = address_list.get(); addr != nullptr; addr = addr->ai_next) {
            int fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
            if (fd < 0) {
                error = "failed to create server socket";
                continue;
            }

            int yes = 1;
            status = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
            if (status < 0) {
                close_with_error(fd, error, "failed to set SO_REUSEADDR on server socket");
                continue;
            }

            status = bind(fd, addr->ai_addr, addr->ai_addrlen);
            if (status < 0) {
                close_with_error(fd, error, "failed to bind server socket");
                continue;
            }

            status = ::listen(fd, backlog);
            if (status < 0) {
                close_with_error(fd, error, "failed to listen on server socket");
                continue;
            }

            return fd;
        }

        throw runtime_error(error);
    }
} // namespace nexora
