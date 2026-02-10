#include "Eventloop.h"
#include "Server.h"
#include "iostream"

const int PORT = 8888;

int main() {    
    std::cout << "[Debug] Main: Starting server on port " << PORT << "..." << std::endl;

    try {
        Eventloop* loop = new Eventloop();
        Server* server = new Server("0.0.0.0", PORT, loop);

        loop->loop();

        delete loop;
        delete server;
    } 
    catch (const std::exception& e) {
        std::cerr << "[Fatal Error] Server crashed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}