#include <iostream>
#include <unistd.h>
#include <vector>
#include <fcntl.h>
#include <cerrno>
#include <cstdio>
#include <cstring>

#include "Socket.h"
#include "Epoll.h"

const int PORT = 8888;

int main() {
    // Create Server Address
    // Listen on port 8888, IP 0.0.0.0 (accept connections from any network interface)
    InetAddress serverAddr("0.0.0.0", PORT);
    
    // Create Socket
    Socket servSock;    
    servSock.set_non_blocking();

    servSock.bind(serverAddr);
    servSock.listen();    
    std::cout << "Server is listening on port " << PORT << " ..." << std::endl;

    // Create Epoll
    Epoll epoll;
    epoll.add_fd(servSock.get_fd(), EPOLLIN | EPOLLET);
    
    // Enter infinite loop to accept connections
    while (true) {
        // Prepare an empty address to store client info
        InetAddress clientAddr("0.0.0.0", 0); // Parameters here don't matter, they will be overwritten
        
        std::vector<epoll_event> events = epoll.poll(-1);

        for (auto event : events) {            
            if (event.data.fd == servSock.get_fd()) {
                int clientFd = servSock.accept(&clientAddr);
                if (clientFd != -1) {
                    std::cout << "Connection successful!" << std::endl;

                    // Set client to non-blocking mode
                    fcntl(clientFd, F_SETFL, fcntl(clientFd, F_GETFL) | O_NONBLOCK);
                    epoll.add_fd(clientFd, EPOLLIN | EPOLLET);
                }                
                std::cout << "Client IP: " << inet_ntoa(clientAddr.get_addr()->sin_addr) << std::endl;
            } else {
                int clientFd = event.data.fd;
                char buffer[1024];

                int readBytes = read(clientFd, buffer, sizeof(buffer)); 
                if (readBytes > 0){
                    std::cout << "Received: " << buffer << std::endl;
                    
                    // Simple greeting (write to socket)
                    const char* hello = "Hello from Server!\n";
                    write(clientFd, hello, sizeof(strlen(hello))); // Simple write test
                } else if (readBytes == 0) {
                    // Close client connection and remove from epoll
                    std::cout << "Client disconnected.";
                    epoll.remove_fd(clientFd);      
                    close(clientFd);     
                } else if (readBytes == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("Read error");                    
                    epoll.remove_fd(clientFd);
                    close(clientFd);   
                }       
            }
        }
    }

    return 0;
}