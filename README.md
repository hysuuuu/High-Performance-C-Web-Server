# High-Performance Asynchronous Web Server

![Language](https://img.shields.io/badge/language-C%2B%2B17-blue.svg)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![License](https://img.shields.io/badge/license-MIT-orange.svg)

## Introduction

A high-performance, non-blocking HTTP web server built from scratch using **C++17** and **Systems Programming** techniques on Linux.

This project implements the **Reactor Pattern** with **Edge-Triggered epoll** to handle massive concurrent connections with minimal resource consumption. It features a custom **Thread Pool** for computational tasks and a **Finite State Machine (FSM)** for efficient HTTP parsing.

**Current Status:** _Active Development (Prototyping Core Network Layer)_

## Key Features (Targeted)

- **Event-Driven Architecture:** Utilizes Linux `epoll` (Edge-Triggered mode) for efficient I/O multiplexing, capable of handling **10,000+ concurrent connections**.
- **Multi-Threading:** Implements a **Thread Pool** pattern to decouple connection handling from request processing, maximizing CPU core utilization.
- **Zero-Copy Parsing:** Custom HTTP request parser based on a Finite State Machine to minimize memory allocations.
- **Robust Resource Management:** Strictly follows **RAII** principles with C++17 smart pointers (`std::unique_ptr`, `std::shared_ptr`) to ensure no memory leaks.
- **High Performance:** Optimized task queue with `std::mutex` and `std::condition_variable` to reduce context-switching overhead.
- **Production Ready:** Containerized with **Docker** for consistent deployment.

## System Architecture

The server adopts a **"One Loop Per Thread"** model inspired by the Reactor pattern.

```text
+----------------+      +------------------+
|  Main Reactor  | <--> |  Acceptor (Listen)|
|   (Main Loop)  |      +------------------+
+-------+--------+
        |
        | Dispatch (Round Robin)
        v
+-------+--------+      +------------------+
|  Sub Reactor   | <--> |  Event Loop      |
|    (Thread)    |      |  (epoll wait)    |
+-------+--------+      +------------------+
        |
        | Add Task
        v
+-------+--------+      +------------------+
|  Worker Thread | <--> |  Business Logic  |
|     Pool       |      |  (HTTP/DB)       |
+----------------+      +------------------+
```
