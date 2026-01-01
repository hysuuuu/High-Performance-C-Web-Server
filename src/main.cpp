#include "Eventloop.h"
#include "Server.h"

const int PORT = 8888;

int main() {    
    Eventloop* loop = new Eventloop();
    Server* server = new Server("0.0.0.0", PORT, loop);
        
    // Enter infinite loop to accept connections
    loop->loop();

    delete loop;
    delete server;
    return 0;
}