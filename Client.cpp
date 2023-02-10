#include "Client.h"
#include "ClientManager.h"


Client::Client(std::shared_ptr<ClientManager> manager, int socket) : manager(std::move(manager)), socket(socket) {}

Client::~Client() {
    shutdown(socket, SHUT_RDWR);
    close(socket);
}

void Client::handleError() {
    manager->removeClient(socket);
}

int Client::getSocket() const {
    return socket;
}

void Client::sendPacketToClient(Packet& packet) {
    if (sendPacket(socket, packet) == -1) {
        handleError();
    }
}

std::optional<Packet> Client::receivePacketFromClient() {
    Packet packet = receivePacket(socket);

    if (packet.packetType == PacketType::END) {
        handleError();
        return {};
    }

    return packet;
}