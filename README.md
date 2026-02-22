# High-Performance Asynchronous Web Server

![Language](https://img.shields.io/badge/language-C%2B%2B17-blue.svg)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![License](https://img.shields.io/badge/license-MIT-orange.svg)

## Introduction

A non-blocking HTTP/1.1 web server built from scratch using C++17 on Linux.

The current implementation is a single-reactor, edge-triggered epoll server with a worker thread pool for request processing. It serves static files from the local www directory and uses picohttpparser for lightweight HTTP parsing.

**Current Status:** Active development. Single event loop + thread pool + static file server are working. Multi-loop reactor and advanced routing are not implemented yet.

## Key Features (Current)

- Event-driven I/O with edge-triggered epoll and non-blocking sockets.
- Connection management via Channel/Connection classes, cleaned up with RAII patterns.
- Thread pool for parsing and response generation on worker threads.
- HTTP/1.1 parsing via picohttpparser.
- Keep-Alive support plus idle timeout sweeping (default 30 seconds).
- Static file serving with basic MIME type detection.

## System Architecture

The server currently uses a single event loop and a worker pool for request processing:

```text
+------------------+      +------------------+
|  Acceptor        | <--> |  Event Loop      |
|  (listen socket) |      |  (epoll wait)    |
+--------+---------+      +--------+---------+
         |                         |
         |                         | read/write events
         v                         v
+--------+---------+      +------------------+
|  Connection      | <--> |  Buffer + Channel|
+--------+---------+      +------------------+
         |
         | dispatch request
         v
+------------------+
| Worker Threadpool|
+------------------+
```

## HTTP Parsing

The network and buffering layers are custom, while HTTP parsing uses picohttpparser for low overhead, zero-copy style request parsing.

## Build and Run

```bash
mkdir -p build
cd build
cmake ..
make -j
./server
```

Server listens on port 8888 by default. Access http://localhost:8888/ to load www/index.html.

## Tests

```bash
cd build
ctest
```

## Docker

```bash
docker build -t hpcpp-server .
docker run --rm -p 8888:8888 hpcpp-server
```
