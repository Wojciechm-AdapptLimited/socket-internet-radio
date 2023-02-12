#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <cstring>
#include <atomic>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "MusicPlayer.h"
#include "Packet.h"


void handlePackets(int socket, std::atomic<bool>& clientRunning, NetworkTraffic& networkTraffic) {
    while (clientRunning) {
        {
            Packet packetToReceive{};
            receivePacket(socket, packetToReceive);
            std::lock_guard<std::mutex> guard_read(networkTraffic.recvMutex);
            networkTraffic.received.push(packetToReceive);
        }

        std::lock_guard<std::mutex> guard_read(networkTraffic.sentMutex);

        if (networkTraffic.sent.empty())
            continue;

        while (!networkTraffic.sent.empty()) {
            Packet packet = networkTraffic.sent.front();
            networkTraffic.sent.pop();
            sendPacket(socket, packet);
            freePacket(packet);
        }
    }
}

void putPacketOnQueue(const Packet& packet, NetworkTraffic& networkTraffic) {
    std::lock_guard<std::mutex> guard(networkTraffic.sentMutex);
    networkTraffic.sent.push(packet);
}

void handleMusicStreamPacket(MusicPlayer& musicPlayer, const Packet& musicStreamPacket) {
    musicPlayer.fetchAudioToMemory(musicStreamPacket.data, musicStreamPacket.dataSize);
}

void handleMusicStreamNamePacket(MusicPlayer& musicPlayer, const Packet& musicStreamNamePacket) {
    std::string newName(musicStreamNamePacket.data);
    musicPlayer.setSongName(newName);
}

void handleMusicStreamSizePacket(MusicPlayer& musicPlayer, const Packet& musicStreamSizePacket) {
    int musicFileSize = 0;
    std::memcpy(&musicFileSize, musicStreamSizePacket.data, musicStreamSizePacket.dataSize);
    musicPlayer.startFetchAudio(musicFileSize);
}

void processPacketsReceived(std::atomic<bool>& clientRunning, std::atomic<bool>& songPlaying,
                            std::atomic<bool>& shouldPrintMainMenu, NetworkTraffic& networkTraffic,
                            MusicPlayer& musicPlayer) {
    while (clientRunning) {
        std::lock_guard<std::mutex> guard(networkTraffic.recvMutex);
        while (!networkTraffic.received.empty()) {
            Packet& packet = networkTraffic.received.front();
            if (packet.packetType == PacketType::STREAM_SIZE) {
                handleMusicStreamSizePacket(musicPlayer, packet);
                songPlaying = false;
                freePacket(packet);
                networkTraffic.received.pop();
            } else if (packet.packetType == PacketType::STREAM) {
                handleMusicStreamPacket(musicPlayer, packet);
                freePacket(packet);
                networkTraffic.received.pop();

                if (musicPlayer.readyToPlayMusic() && !songPlaying) {
                    musicPlayer.playMusic();
                    songPlaying = true;
                    shouldPrintMainMenu = true;
                }
            } else if (packet.packetType == PacketType::STREAM_NAME) {
                handleMusicStreamNamePacket(musicPlayer, packet);
                freePacket(packet);
                networkTraffic.received.pop();
            } else if (packet.packetType == PacketType::STREAM_HEADER) {
                handleMusicStreamPacket(musicPlayer, packet);
                freePacket(packet);
                networkTraffic.received.pop();
            }
        }
    }
}

void printMainMenu() {
    std::cout << "***********\n";
    std::cout << "S: Request a new song\n";
    std::cout << "Q: Quit\n";
    std::cout << "***********\n";
}


int main() {
    std::atomic<bool> clientRunning = {true};
    std::atomic<bool> songPlaying = {false};
    std::atomic<bool> shouldPrintMainMenu = {false};
    NetworkTraffic networkTraffic;
    MusicPlayer musicPlayer;

    initEngine();

    int clientSocket;
    std::string serverIp = "127.0.0.1";
    struct sockaddr_in serverAddr{AF_INET, htons(1100)};

    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    clientSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Failed to create socket\n";
        shutdownEngine();
        return EXIT_FAILURE;
    }

    if (inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported\n";
        shutdownEngine();
        return EXIT_FAILURE;
    }

    int connectRes = connect(clientSocket, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
    if (connectRes < 0) {
        std::cerr << "Connection failed\n";
        shutdownEngine();
        return EXIT_FAILURE;
    }


    musicPlayer.setSongName("test");

    std::thread threadHandlePackets(handlePackets, clientSocket, std::ref(clientRunning), std::ref(networkTraffic));

    std::thread threadProcessPackets(processPacketsReceived, std::ref(clientRunning), std::ref(songPlaying),
                                     std::ref(shouldPrintMainMenu), std::ref(networkTraffic), std::ref(musicPlayer));

    bool userExited = false;
    char input;
    while (!userExited) {
        if (shouldPrintMainMenu) {
            shouldPrintMainMenu = false;
            printMainMenu();
            std::cin >> input;

            if (input == 'Q') {
                userExited = true;
            }
        }
    }

    // Inform server that we have disconnected
    Packet disconnect{PacketType::END, 0, nullptr};
    putPacketOnQueue(disconnect, networkTraffic);

    bool disconnectPacketSent = false;
    while (!disconnectPacketSent) {
        std::lock_guard<std::mutex> guard(networkTraffic.sentMutex);
        if (networkTraffic.sent.empty()) {
            disconnectPacketSent = true;
        }
    }

    clientRunning = false;

    threadProcessPackets.join();
    threadHandlePackets.join();

    shutdownEngine();

    close(clientSocket);

    return 0;
}
