#include <iostream>
#include <csignal>

#include "Eventloop.h"
#include "Server.h"


const int PORT = 8888;

Eventloop* g_loop = nullptr;

void signal_handler(int sig) {
    std::cout << "\n[Info] Received signal " << sig << " (Ctrl+C). Shutting down gracefully..." << std::endl;
    if (g_loop) {
        g_loop->quit(); 
    }
}

int main() {    
    std::cout << "[Debug] Main: Starting server on port " << PORT << "..." << std::endl;

    std::signal(SIGINT, signal_handler);  // catch Ctrl+C
    std::signal(SIGTERM, signal_handler); // catch kill 

    try {       
        Eventloop loop;
        g_loop = &loop;
        Server server("0.0.0.0", PORT, &loop);

        loop.loop();
    } 
    catch (const std::exception& e) {
        std::cerr << "[Fatal Error] Server crashed: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "[Info] Server successfully stopped. Goodbye!" << std::endl;
    return 0;
}