#include "Packet.h"


void freePacket(Packet& packet) {
    if(packet.dataSize > 0)
    {
        delete[] packet.data;
    }
}

std::size_t sendPacket(int socket, const Packet& packet) {
    char sendBuffer[BUFFER_SIZE];
    size_t offset = 0;

    memcpy(sendBuffer + offset, &packet.packetType, sizeof(packet.packetType));
    offset += sizeof(packet.packetType);

    memcpy(sendBuffer + offset, &packet.dataSize, sizeof(packet.dataSize));
    offset += sizeof(packet.dataSize);

    if (packet.dataSize > 0) {
        memcpy(sendBuffer + offset, packet.data, DATA_SIZE);
    }

    std::size_t sent = send(socket, sendBuffer, BUFFER_SIZE, 0);

    return sent;
}

void receivePacket(int socket, Packet& packet) {
    char receiveBuffer[BUFFER_SIZE];
    std::size_t received = recv(socket, receiveBuffer, BUFFER_SIZE, 0);

    if (received == -1) {
        packet.packetType = PacketType::END;
        packet.dataSize = 0;
        packet.data = nullptr;
        return;
    }

    std::size_t curPos = received;

    while(received < BUFFER_SIZE)
    {
        received += recv(socket, receiveBuffer + curPos, BUFFER_SIZE - curPos, 0);
        curPos = received;

        if(received == 0)
        {
            break;
        }
    }

    size_t offset = 0;
    memcpy(&packet.packetType, receiveBuffer + offset, sizeof(packet.packetType));
    offset += sizeof(packet.packetType);
    memcpy(&packet.dataSize, receiveBuffer + offset, sizeof(packet.dataSize));
    offset += sizeof(packet.dataSize);

    if(packet.dataSize > 0)
    {
        packet.data = new char[packet.dataSize];
        memcpy(packet.data, receiveBuffer + offset, packet.dataSize);
    }
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
    size_t pos;
    std::vector<std::string> vec;

    while ((pos = s.find(delimiter)) != std::string::npos) {
        vec.push_back(s.substr(0, pos));
        s.erase(0, pos + delimiter.length());
    }
    vec.push_back(s);

    return vec;
}
