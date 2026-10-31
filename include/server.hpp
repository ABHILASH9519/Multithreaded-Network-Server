#pragma once
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(std::size_t workers);
    ~ThreadPool();
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    void submit(int client_fd);
    void shutdown();
private:
    void worker_loop();
    std::mutex mutex_;
    std::condition_variable ready_;
    std::queue<int> jobs_;
    std::vector<std::thread> workers_;
    bool stopping_{false};
};

class TcpServer {
public:
    TcpServer(std::string host, std::uint16_t port, std::size_t workers);
    ~TcpServer();
    int run();
    void stop();
private:
    std::string host_;
    std::uint16_t port_;
    ThreadPool pool_;
    std::atomic<bool> stopping_{false};
    std::atomic<int> listen_fd_{-1};
};