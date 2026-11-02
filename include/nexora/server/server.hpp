#pragma once

namespace nexora {
    class Server {
    public:
        Server() = default;
        ~Server();

        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;

        void listen(int port);

    private:
        static int create_listener(int port);
        static void close_with_error(int& fd, const char*& error, const char* message);

        int fd_ = -1;
    };

} // namespace nexora
