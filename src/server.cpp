#include "DatabaseManager.h"
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>

const int PORT = 8888;

struct ClientArgs {
    int sock;
    DatabaseManager* db;
};

void* handle_client(void* arg) {
    ClientArgs* args = (ClientArgs*)arg;
    int sock = args->sock;
    DatabaseManager* db = args->db;
    delete args;

    char buffer[4096];
    while (true) {
        ssize_t n = recv(sock, buffer, sizeof(buffer)-1, 0);
        if (n <= 0) break;
        buffer[n] = 0;

        std::string cmd(buffer);
        
        if (cmd == "EXIT\n" || cmd == "EXIT\r\n" || cmd == "exit\n" || cmd == "exit\r\n")
            break;

        while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r')) cmd.pop_back();

        std::ostringstream response;
        std::streambuf* old_cout = std::cout.rdbuf();
        std::cout.rdbuf(response.rdbuf());
        bool ok = db->exec(cmd);
        std::cout.rdbuf(old_cout);

        std::string resp = response.str();
        if (!ok && resp.empty())
            resp = "ERROR: Unsupported or invalid command\n";
        if (resp.empty())
            resp = "OK\n";
        send(sock, resp.c_str(), resp.size(), 0);
    }
    close(sock);
    pthread_exit(nullptr);
}

int main() {
    mkdir("./data", 0755);
    DatabaseManager db("./data");

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 2;
    }
    if (listen(server_fd, 16) < 0) {
        perror("listen");
        return 3;
    }
    std::cout << "OurSQL DB server running on port " << PORT << std::endl;

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) { perror("accept"); continue; }
        ClientArgs* args = new ClientArgs{client_fd, &db};
        pthread_t tid;
        pthread_create(&tid, nullptr, handle_client, args);
        pthread_detach(tid);
    }
    close(server_fd);
    return 0;
}
