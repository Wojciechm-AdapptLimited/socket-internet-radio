#include <filesystem>
#include <vector>
#include <optional>
#include <mutex>
#include <cstring>
#include <sys/socket.h>
#include <queue>

#ifndef INTERNET_RADIO_REQUEST_H
#define INTERNET_RADIO_REQUEST_H

enum class PacketType : int {
    STREAM, STREAM_SIZE, STREAM_NAME, STREAM_HEADER, SKIP, ADD, REMOVE, REORDER, END, FILES, QUEUE
};

struct Packet {
    PacketType packetType;
    unsigned int dataSize;
    char* data;
};

struct NetworkTraffic {
    std::queue<Packet> received;
    std::queue<Packet> sent;
    std::mutex recvMutex;
    std::mutex sentMutex;
};


constexpr int BUFFER_SIZE = 1024;
constexpr int DATA_SIZE = BUFFER_SIZE - sizeof(Packet) + sizeof(char*);

std::size_t sendPacket(int socket, const Packet& packet);
void receivePacket(int socket, Packet &packet);
void freePacket(Packet& packet);
std::vector<std::filesystem::path> parseRequest(Packet& packet);
std::vector<std::string> split(std::string s, const std::string& delimiter);

#endif //INTERNET_RADIO_REQUEST_H
