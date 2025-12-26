#include <iostream>
#include <unistd.h>
#include <vector>
#include <fcntl.h>
#include <cerrno>
#include <cstdio>
#include <cstring>

#include "Socket.h"
#include "InetAddress.h"
#include "Epoll.h"
#include "Channel.h"
#include "Eventloop.h"

const int PORT = 8888;

void handle_disconnect(Epoll* ep, int fd, Channel* ch) {
    ep->remove_fd(fd);      
    close(fd);   
    delete ch;    
}

int main() {
    // Create Server Address
    // Listen on port 8888, IP 0.0.0.0 (accept connections from any network interface)
    InetAddress server_addr("0.0.0.0", PORT);

    // Prepare an empty address to store client info
    InetAddress client_addr("0.0.0.0", 0); 
    
    // Create Socket
    Socket serv_sock;    
    serv_sock.set_non_blocking();

    serv_sock.bind(server_addr);
    serv_sock.listen();    
    std::cout << "Server is listening on port " << PORT << " ..." << std::endl;

    // Create Loop and Channel
    Eventloop* loop = new Eventloop();
    Channel* serv_ch = new Channel(loop, serv_sock.get_fd());
    serv_ch->enable_reading();

    // Handle new incoming connection requests
    std::function<void()> handle_accept = [&] () {
        Channel* client_ch = new Channel(loop, serv_sock.accept(&client_addr));
        int client_fd = client_ch->get_fd();

        if (client_fd != -1) {
            std::cout << "Connection successful!" << std::endl;

            std::function<void()> handle_read = [client_fd, &loop, &client_ch] () {
                Epoll* epoll = loop->get_epoll();
                char buffer[1024];
                int read_bytes = read(client_fd, buffer, sizeof(buffer)); 

                while (read_bytes > 0) {
                    std::cout << "Received: " << buffer << std::endl;
                    read_bytes = read(client_fd, buffer, sizeof(buffer)); 
                    
                    // Simple greeting (write to socket)
                    const char* hello = "Hello from Server!\n";
                    write(client_fd, hello, strlen(hello));            
                }
                if (read_bytes == 0) {
                    // Close client connection and remove from epoll
                    std::cout << "Client disconnected.";
                    handle_disconnect(epoll, client_fd, client_ch);
                } else if (read_bytes == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    } else {
                        perror("Read error");    
                        handle_disconnect(epoll, client_fd, client_ch);                
                    }                      
                }       
            };

            // Set client to non-blocking mode
            fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL) | O_NONBLOCK);                
            client_ch->set_readCallback(handle_read);
            client_ch->enable_reading();
        }  
        std::cout << "Client IP: " << inet_ntoa(client_addr.get_addr()->sin_addr) << std::endl;          
    };        
    serv_ch->set_readCallback(handle_accept);   
        
    // Enter infinite loop to accept connections
    loop->loop();

    return 0;
}