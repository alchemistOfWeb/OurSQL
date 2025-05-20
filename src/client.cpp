#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

const int PORT = 8888;

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return 2;
    }

    std::cout << "Connected to OurSQLDB server at 127.0.0.1:" << PORT << "\n";
    std::cout << "Type SQL and press Enter. Type EXIT to quit.\n";

    char buffer[8192];
    while (true) {
        std::cout << "> ";
        std::string line;
        if (!std::getline(std::cin, line)) break;
        line += '\n';

        if (send(sock, line.c_str(), line.size(), 0) < 0) {
            perror("send");
            break;
        }
        if (line == "EXIT\n" || line == "exit\n") break;

        ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            std::cout << "Server disconnected.\n";
            break;
        }
        buffer[n] = '\0';
        std::cout << buffer;
    }
    close(sock);
    return 0;
}
