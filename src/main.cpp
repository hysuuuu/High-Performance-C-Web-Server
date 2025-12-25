#include <iostream>
#include "InetAddress.h"
#include "Socket.h"
#include <unistd.h> // 為了使用 close()

int main() {
    // 1. 建立 Server 地址 (房東)
    // 監聽 8888 port，IP 設為 0.0.0.0 (代表接收所有網卡的連線)
    InetAddress serverAddr("0.0.0.0", 8888);
    
    // 2. 建立 Socket (手機)
    Socket servSock;
    
    // 3. 綁定地址 (插上 SIM 卡)
    servSock.bind(serverAddr);
    
    // 4. 開始監聽 (開機待機)
    servSock.listen();
    
    std::cout << "Server is listening on port 8888..." << std::endl;

    // 5. 進入無窮迴圈接客
    while (true) {
        // 準備一個空的地址來裝客人的資料
        InetAddress clientAddr("0.0.0.0", 0); // 這裡的參數隨便填，反正會被覆蓋
        
        // 阻塞在這裡，直到有人連線
        int clientFd = servSock.accept(&clientAddr);
        
        if (clientFd != -1) {
            std::cout << "Connection successful!" << std::endl;
            
            // 測試看看有沒有抓到 IP
            // (這需要我們去 InetAddress 加一個 getIp()，如果還沒寫沒關係，先略過)
            std::cout << "Client IP: " << inet_ntoa(clientAddr.get_addr()->sin_addr) << std::endl;
            
            char buffer[1024];
            // 這裡會發生阻塞！因為 Client 睡了 1 秒才送資料
            // Server 必須在這裡傻等 1 秒，無法處理其他人的連線
            int readBytes = read(clientFd, buffer, sizeof(buffer)); 
            if(readBytes > 0){
                std::cout << "Received: " << buffer << std::endl;
            }
            // ======

            // 簡單打個招呼 (寫入 socket)
            const char* hello = "Hello from C++ Server!\n";
            write(clientFd, hello, sizeof(hello)); // 簡單的寫入測試

            // 關閉這個客人的連線 (因為我們只是測試，講一句話就掛電話)
            close(clientFd);
        }
    }

    return 0;
}