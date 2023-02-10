#include "Packet.h"


void freePacket(Packet& packet) {
    if(packet.dataSize > 0)
    {
        delete[] packet.data;
    }
}

std::size_t sendPacket(int socket, Packet& packet) {
    std::vector<char> sendBuffer(BUFFER_SIZE);
    size_t offset = 0;

    std::memcpy(sendBuffer.data() + offset, &packet.packetType, sizeof(packet.packetType));
    offset += sizeof(packet.packetType);

    std::memcpy(sendBuffer.data() + offset, &packet.dataSize, sizeof(packet.dataSize));
    offset += sizeof(packet.dataSize);

    if (packet.dataSize > 0) {
        std::memcpy(sendBuffer.data() + offset, &packet.data, DATA_SIZE);
    }

    std::size_t sent = send(socket, sendBuffer.data(), BUFFER_SIZE, 0);

    return sent;
}

Packet receivePacket(int socket) {
    std::vector<char> requestBuffer(BUFFER_SIZE);
    std::size_t received = recv(socket, requestBuffer.data(), BUFFER_SIZE, 0);

    if (received == -1) {
        Packet packet {PacketType::END, 0, nullptr};
        return packet;
    }

    std::size_t curPos = received;

    while(received < BUFFER_SIZE)
    {
        received += recv(socket, requestBuffer.data() + curPos, BUFFER_SIZE - curPos, 0);
        curPos = received;

        if(received == 0)
        {
            break;
        }
    }

    Packet packet {};

    size_t offset = 0;
    std::memcpy(&packet.packetType, requestBuffer.data() + offset, sizeof(packet.packetType));
    offset += sizeof(packet.packetType);
    std::memcpy(&packet.dataSize, requestBuffer.data() + offset, sizeof(packet.dataSize));
    offset += sizeof(packet.dataSize);

    if(packet.dataSize > 0)
    {
        packet.data = new char[packet.dataSize];
        memcpy(packet.data, requestBuffer.data() + offset, packet.dataSize);
    }

    return packet;
}

std::vector<std::filesystem::path> parseRequest(Packet& packet) {
    std::vector<std::filesystem::path> files;

    if (packet.packetType != PacketType::SKIP) {
        std::string fileList {packet.data};
        std::vector<std::string> fileNamesStr = split(fileList, ";");
        std::transform(fileNamesStr.begin(), fileNamesStr.end(), std::back_inserter(files), [&](const std::string& s) {
            return std::filesystem::path{s};
        });
    }

    return files;
}

std::vector<std::string> split(std::string s, const std::string& delimiter) {
    size_t pos = 0;
    std::vector<std::string> vec;

    while ((pos = s.find(delimiter)) != std::string::npos) {
        vec.push_back(s.substr(0, pos));
        s.erase(0, pos + delimiter.length());
    }
    vec.push_back(s);

    return vec;
}
