#include "server.hpp"

#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
std::string exchange(ThreadPool& pool, const std::string& request) {
    int sockets[2] = {-1, -1};
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
        throw std::runtime_error("socketpair failed");
    }

    pool.submit(sockets[0]);
    std::size_t sent = 0;
    while (sent < request.size()) {
        const auto count = ::send(sockets[1], request.data() + sent,
                                  request.size() - sent, 0);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) {
            ::close(sockets[1]);
            throw std::runtime_error("request send failed");
        }
        sent += static_cast<std::size_t>(count);
    }
    ::shutdown(sockets[1], SHUT_WR);

    std::string response;
    char buffer[256];
    for (;;) {
        const auto count = ::recv(sockets[1], buffer, sizeof(buffer), 0);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) break;
        response.append(buffer, static_cast<std::size_t>(count));
    }
    ::close(sockets[1]);
    return response;
}
}

int main() {
    bool rejected = false;
    try {
        ThreadPool invalid(0);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);

    ThreadPool pool(2);
    assert(exchange(pool, "hello server\n") == "OK: hello server\n");
    assert(exchange(pool, "\n") == "ERR: empty request\n");

    const std::string oversized(4097, 'x');
    assert(exchange(pool, oversized + "\n") == "ERR: request too large\n");

    pool.shutdown();
    pool.shutdown(); // shutdown is intentionally idempotent
    std::cout << "server core tests passed\n";
}
