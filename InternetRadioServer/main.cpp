#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <csignal>
#include "AudioStreamer.h"
#include "ClientManager.h"

std::atomic<bool> g_exit_program{false};

void sig_handler(int signum) {
    if (signum == SIGTERM) {
        std::cout << "Received SIGTERM" << std::endl;
        g_exit_program = true;
    }
}

int main() {
    int serverSocket;
    struct sockaddr_in serverAddr {AF_INET, htons(1100)};
    struct sockaddr_storage serverStorage{};
    socklen_t addr_size;

    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create socket";
        return EXIT_FAILURE;
    }

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof serverAddr) != 0) {
        std::cerr << "Failed to bind\n";
        close(serverSocket);
        return EXIT_FAILURE;
    }

    if (listen(serverSocket, 50) != 0) {
        std::cerr << "Failed to listen\n";
        close(serverSocket);
        return EXIT_FAILURE;
    }

    std::shared_ptr<AudioStreamer> streamer = std::make_shared<AudioStreamer>();
    std::shared_ptr<ClientManager> manager = std::make_shared<ClientManager>(streamer);

    streamer->readDirectory("/home/wojciech/Studies/Networks/internet-radio/audio");

    struct sigaction sa{};
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);

    struct pollfd pollFd = {serverSocket, POLLIN, 0};

    std::thread streamerThread(&AudioStreamer::streamAudioQueue, streamer);
    std::thread managerThread(&ClientManager::handleRequests, manager);
    std::thread acceptClientsThread([&manager, &serverSocket, &addr_size, &serverStorage, &pollFd]{
        while (!g_exit_program) {
            int result = poll(&pollFd, 1, 10000);

            if (result < 0) {
                break;
            }
            else if (result == 0) {
                continue;
            }

            if (pollFd.events & POLLIN) {
                addr_size = sizeof serverStorage;
                int newSocket = accept(serverSocket, (struct sockaddr *)&serverStorage, &addr_size);
                if(newSocket > 0) {
                    auto newClient = std::make_unique<Client>(manager->getPtr(), newSocket);
                    pollfd newPollFd = {newSocket, POLLIN | POLLHUP | POLLERR | POLLOUT, 0};
                    manager->addClient(newClient, newPollFd);
                }
            }
        }
    });

    acceptClientsThread.join();

    manager->closeServer = true;
    manager->closeClients();
    streamer->finishStreaming = true;

    streamerThread.join();
    managerThread.join();

    shutdown(serverSocket, SHUT_RDWR);
    close(serverSocket);

    return EXIT_SUCCESS;
}
