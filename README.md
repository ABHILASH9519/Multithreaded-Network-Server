# Multithreaded Network Server (C++)

A compact TCP server demonstrating socket programming, bounded concurrency, line-oriented request handling, and worker-pool lifecycle management.

## Features

- IPv4/IPv6 address resolution with `getaddrinfo`
- Fixed-size worker pool to bound concurrent client handling
- Newline-delimited request protocol with a 4 KiB payload limit
- Empty and oversized request handling
- Signal-triggered server shutdown path
- CMake build, CTest tests, and GitHub Actions CI

## Build and run (Linux/macOS)

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/network_server 127.0.0.1 8080 4
```

In another terminal, connect with `nc 127.0.0.1 8080`, type a line and press Enter. The server responds with `OK: <line>`.

## Protocol

The current protocol handles one newline-terminated request per TCP connection. The maximum payload is 4096 bytes. Empty requests receive an error response; requests exceeding the limit receive `ERR: request too large`.

## Tests

The CTest executable exercises the worker pool through local socket pairs, covering a normal request, an empty request, an oversized request, invalid zero-worker configuration, and repeated shutdown. These are local process tests; they do not claim external-network or throughput validation.

## Scope and production considerations

This repository is an educational systems-programming project, not an internet-facing hardened service. Before production use, add connection deadlines, bounded queue/backpressure policy, structured logging, protocol versioning, authentication where needed, and repeatable load testing. The current implementation uses a simple line protocol and does not provide TLS.
