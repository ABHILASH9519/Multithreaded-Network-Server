# Multithreaded Network Server (C++)

A compact TCP server demonstrating socket programming, bounded concurrency, line-oriented request handling, and graceful shutdown.

## Features
- IPv4/IPv6 address resolution with `getaddrinfo`
- Fixed-size worker pool to bound concurrency
- Newline-delimited request protocol with a 4 KiB frame limit
- Graceful shutdown and deterministic worker cleanup
- CMake build, CTest tests, and GitHub Actions CI

## Build and run (Linux/macOS)
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/network_server 127.0.0.1 8080 4
```

In another terminal: `nc 127.0.0.1 8080`, type a line and press Enter. The server responds with `OK: <line>`.

## Protocol
One request per TCP connection, terminated by newline. Maximum payload: 4096 bytes. Empty requests receive an error response. This is an educational baseline, not an internet-facing hardened server; add timeouts, authentication, rate limiting and structured observability before production use.

## Tests
CTest currently covers worker-pool construction and invalid configuration. Socket-level integration and load benchmarks are planned follow-up work; no throughput percentage is claimed without repeatable benchmark evidence.