#include "server.hpp"
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace {
constexpr std::size_t kMaxLine = 4096;

void handle_client(int fd) {
    std::string line;
    char ch{};
    bool terminated = false;
    while (line.size() <= kMaxLine) {
        const auto n = ::recv(fd, &ch, 1, 0);
        if (n <= 0) break;
        if (ch == '\n') { terminated = true; break; }
        if (ch != '\r') line.push_back(ch);
    }
    std::string response;
    if (!terminated && line.size() > kMaxLine) response = "ERR: request too large\n";
    else if (line.empty()) response = "ERR: empty request\n";
    else response = "OK: " + line + "\n";
    (void)::send(fd, response.data(), response.size(), MSG_NOSIGNAL);
    ::shutdown(fd, SHUT_RDWR);
    ::close(fd);
}
}

ThreadPool::ThreadPool(std::size_t workers) {
    if (workers == 0) throw std::invalid_argument("worker count must be positive");
    for (std::size_t i = 0; i < workers; ++i)
        workers_.emplace_back([this] { worker_loop(); });
}
ThreadPool::~ThreadPool() { shutdown(); }
void ThreadPool::submit(int fd) {
    std::lock_guard lock(mutex_);
    if (stopping_) { ::close(fd); return; }
    jobs_.push(fd);
    ready_.notify_one();
}
void ThreadPool::shutdown() {
    {
        std::lock_guard lock(mutex_);
        if (stopping_) return;
        stopping_ = true;
    }
    ready_.notify_all();
    for (auto& worker : workers_) if (worker.joinable()) worker.join();
    workers_.clear();
    while (!jobs_.empty()) { ::close(jobs_.front()); jobs_.pop(); }
}
void ThreadPool::worker_loop() {
    for (;;) {
        int fd = -1;
        {
            std::unique_lock lock(mutex_);
            ready_.wait(lock, [this] { return stopping_ || !jobs_.empty(); });
            if (stopping_ && jobs_.empty()) return;
            fd = jobs_.front(); jobs_.pop();
        }
        handle_client(fd);
    }
}

TcpServer::TcpServer(std::string host, std::uint16_t port, std::size_t workers)
    : host_(std::move(host)), port_(port), pool_(workers) {}
TcpServer::~TcpServer() { stop(); }
int TcpServer::run() {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    addrinfo* result = nullptr;
    const auto service = std::to_string(port_);
    const int gai = ::getaddrinfo(host_.c_str(), service.c_str(), &hints, &result);
    if (gai != 0) throw std::runtime_error(::gai_strerror(gai));
    int server_fd = -1;
    for (auto* p = result; p; p = p->ai_next) {
        server_fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (server_fd < 0) continue;
        int yes = 1;
        ::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        if (::bind(server_fd, p->ai_addr, p->ai_addrlen) == 0 && ::listen(server_fd, 128) == 0) break;
        ::close(server_fd);
        server_fd = -1;
    }
    ::freeaddrinfo(result);
    if (server_fd < 0) throw std::runtime_error("could not bind/listen");
    listen_fd_ = server_fd;
    while (!stopping_) {
        const int client = ::accept(server_fd, nullptr, nullptr);
        if (client < 0) {
            if (errno == EINTR) continue;
            if (stopping_) break;
            throw std::runtime_error(std::strerror(errno));
        }
        pool_.submit(client);
    }
    pool_.shutdown();
    return 0;
}
void TcpServer::stop() {
    stopping_ = true;
    const int fd = listen_fd_.exchange(-1);
    if (fd >= 0) { ::shutdown(fd, SHUT_RDWR); ::close(fd); }
}