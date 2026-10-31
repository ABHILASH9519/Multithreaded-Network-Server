#include "server.hpp"
#include <csignal>
#include <iostream>
#include <string>

namespace {
TcpServer* active_server = nullptr;
void on_signal(int) { if (active_server) active_server->stop(); }
}
int main(int argc, char** argv) {
    const std::string host = argc > 1 ? argv[1] : "127.0.0.1";
    const int port = argc > 2 ? std::stoi(argv[2]) : 8080;
    const int workers = argc > 3 ? std::stoi(argv[3]) : 4;
    if (port < 1 || port > 65535 || workers < 1) {
        std::cerr << "usage: network_server [host] [port 1..65535] [workers > 0]\n";
        return 2;
    }
    try {
        TcpServer server(host, static_cast<std::uint16_t>(port), static_cast<std::size_t>(workers));
        active_server = &server;
        std::signal(SIGINT, on_signal);
        std::signal(SIGTERM, on_signal);
        std::cout << "Listening on " << host << ':' << port << " with " << workers << " workers\n";
        return server.run();
    } catch (const std::exception& e) {
        std::cerr << "server error: " << e.what() << '\n';
        return 1;
    }
}