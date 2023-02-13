#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <cstring>
#include <atomic>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <gtkmm.h>

#include "MusicPlayer.h"
#include "Packet.h"
#include "RadioWindow.h"


struct SessionData {
    std::atomic<bool> songPlaying{false};
    std::atomic<bool> shouldPrintMenu{false};
    std::mutex fileMutex;

    MusicPlayer musicPlayer;
    NetworkTraffic networkTraffic;
    std::vector<std::string> queue;
    std::vector<std::string> availableFiles;
};


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

void handleFileListPacket(SessionData& sessionData, const Packet& musicQueuePacket) {
    std::unique_lock<std::mutex> lock(sessionData.fileMutex);
    std::string filesSerialized{musicQueuePacket.data};
    if (musicQueuePacket.packetType == PacketType::QUEUE) {
        sessionData.queue = split(filesSerialized, ";");
    } else {
        sessionData.availableFiles = split(filesSerialized, ";");
    }
}

void prepareFileRemovePacket(SessionData& sessionData, Packet& packet, int idx) {
    std::unique_lock<std::mutex> lock(sessionData.fileMutex);
    if (idx <= 0 || idx > sessionData.queue.size()) {
        return;
    }
    std::string fileName = sessionData.queue[idx - 1];
    packet.packetType = PacketType::REMOVE;
    packet.dataSize = static_cast<unsigned int>(fileName.size() + 1);
    packet.data = new char[DATA_SIZE];
    memcpy(packet.data, fileName.data(), packet.dataSize);
}

void prepareFileAddPacket(SessionData& sessionData, Packet& packet, int idx) {
    std::unique_lock<std::mutex> lock(sessionData.fileMutex);
    if (idx <= 0 || idx > sessionData.availableFiles.size()) {
        return;
    }
    std::string fileName = sessionData.availableFiles[idx - 1];
    packet.packetType = PacketType::ADD;
    packet.dataSize = static_cast<unsigned int>(fileName.size() + 1);
    packet.data = new char[DATA_SIZE];
    memcpy(packet.data, fileName.data(), packet.dataSize);
}

void prepareModifyOrderPacket(SessionData& sessionData, Packet& packet, const std::vector<int>& indices) {
    std::unique_lock<std::mutex> lock(sessionData.fileMutex);
    std::string message;
    std::set<int> used;
    for(const auto& idx : indices){
        if (idx <= 0 || idx > sessionData.queue.size() || used.contains(idx)) {
            return;
        }
        used.emplace(idx);
        message += sessionData.queue[idx - 1] + ";";
    }

    packet.packetType = PacketType::REORDER;
    packet.dataSize = static_cast<unsigned int>(message.size() + 1);
    packet.data = new char[DATA_SIZE];
    memcpy(packet.data, message.data(), packet.dataSize);
}

void processPacketsReceived(std::atomic<bool>& clientRunning, SessionData& sessionData) {
    while (clientRunning) {
        std::lock_guard<std::mutex> guard(sessionData.networkTraffic.recvMutex);
        while (!sessionData.networkTraffic.received.empty()) {
            Packet& packet = sessionData.networkTraffic.received.front();
            if (packet.packetType == PacketType::STREAM_SIZE) {
                handleMusicStreamSizePacket(sessionData.musicPlayer, packet);
                sessionData.songPlaying = false;
                freePacket(packet);
                sessionData.networkTraffic.received.pop();
            } else if (packet.packetType == PacketType::STREAM) {
                handleMusicStreamPacket(sessionData.musicPlayer, packet);
                freePacket(packet);
                sessionData.networkTraffic.received.pop();

                if (sessionData.musicPlayer.readyToPlayMusic() && !sessionData.songPlaying) {
                    sessionData.musicPlayer.playMusic();
                    sessionData.songPlaying = true;
                    sessionData.shouldPrintMenu = true;
                }
            } else if (packet.packetType == PacketType::STREAM_NAME) {
                handleMusicStreamNamePacket(sessionData.musicPlayer, packet);
                freePacket(packet);
                sessionData.networkTraffic.received.pop();
            } else if (packet.packetType == PacketType::STREAM_HEADER) {
                handleMusicStreamPacket(sessionData.musicPlayer, packet);
                freePacket(packet);
                sessionData.networkTraffic.received.pop();
            } else if (packet.packetType == PacketType::QUEUE || packet.packetType == PacketType::FILES) {
                handleFileListPacket(sessionData, packet);
                freePacket(packet);
                sessionData.networkTraffic.received.pop();
            }
        }
    }
}

void printMainMenu(SessionData& sessionData) {
    std::unique_lock<std::mutex> lock(sessionData.fileMutex);
    std::cout << "***********\n";
    std::cout << "Currently playing: " << sessionData.musicPlayer.getSongName() << "\n";
    std::cout << "Next: " << sessionData.queue.front() << "\n";
    std::cout << "S: Skip the current song\n";
    std::cout << "A: Add song to the queue\n";
    std::cout << "R: Remove the song from the queue\n";
    std::cout << "M: Modify the order of the queue\n";
    std::cout << "Q: Quit\n";
    std::cout << "***********\n";
}

void printQueue(SessionData& sessionData) {
    std::unique_lock<std::mutex> lock(sessionData.fileMutex);
    int idx = 1;
    std::cout << "***********\n";
    for (const auto& file: sessionData.queue) {
        std::cout << idx << " " << file << "\n";
        idx++;
    }
    std::cout << "***********\n";
}

void printAvailableFiles(SessionData& sessionData) {
    std::unique_lock<std::mutex> lock(sessionData.fileMutex);
    int idx = 1;
    std::cout << "***********\n";
    for (const auto& file: sessionData.availableFiles) {
        std::cout << idx << " " << file << "\n";
        idx++;
    }
    std::cout << "***********\n";
}

int main(int argc, char* argv[]) {
    std::atomic<bool> clientRunning = {true};
    SessionData sessionData;

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


    sessionData.musicPlayer.setSongName("test");

    std::thread threadHandlePackets(handlePackets, clientSocket, std::ref(clientRunning),
                                    std::ref(sessionData.networkTraffic));

    std::thread threadProcessPackets(processPacketsReceived, std::ref(clientRunning), std::ref(sessionData));

    bool userExited = false;
    char input;
    int fileIdx;
    std::vector<int> fileIndices;
    while (!userExited) {
        if (sessionData.shouldPrintMenu) {
            sessionData.shouldPrintMenu = false;
            printMainMenu(sessionData);
            std::cin >> input;

            if (input == 'Q') {
                userExited = true;
            } else if (input == 'S') {
                Packet packet{PacketType::SKIP, 0, nullptr};
                putPacketOnQueue(packet, sessionData.networkTraffic);
            } else if (input == 'A' && !sessionData.availableFiles.empty()) {
                printAvailableFiles(sessionData);
                std::cin >> fileIdx;
                Packet packet{};
                prepareFileAddPacket(sessionData, packet, fileIdx);
                if (packet.dataSize > 0) {
                    putPacketOnQueue(packet, sessionData.networkTraffic);
                } else {
                    std::cout << "Invalid index\n";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                sessionData.shouldPrintMenu = true;
            } else if (input == 'R' && !sessionData.queue.empty()) {
                printQueue(sessionData);
                std::cin >> fileIdx;
                Packet packet{};
                prepareFileRemovePacket(sessionData, packet, fileIdx);
                if (packet.dataSize > 0) {
                    putPacketOnQueue(packet, sessionData.networkTraffic);
                } else {
                    std::cout << "Invalid index\n";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                sessionData.shouldPrintMenu = true;
            } else if (input == 'M' && !sessionData.queue.empty()) {
                printQueue(sessionData);
                fileIndices.clear();
                for (int i = 0; i < sessionData.queue.size(); i++) {
                    std::cin >> fileIdx;
                    fileIndices.emplace_back(fileIdx);
                }
                Packet packet{};
                prepareModifyOrderPacket(sessionData, packet, fileIndices);
                if (packet.dataSize > 0) {
                    putPacketOnQueue(packet, sessionData.networkTraffic);
                } else {
                    std::cout << "Invalid index\n";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                sessionData.shouldPrintMenu = true;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                sessionData.shouldPrintMenu = true;
            }
        }
    }

    // Inform server that we have disconnected
    Packet disconnect{PacketType::END, 0, nullptr};
    putPacketOnQueue(disconnect, sessionData.networkTraffic);

    bool disconnectPacketSent = false;
    while (!disconnectPacketSent) {
        std::lock_guard<std::mutex> guard(sessionData.networkTraffic.sentMutex);
        if (sessionData.networkTraffic.sent.empty()) {
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
