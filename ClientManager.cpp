#include "ClientManager.h"
#include "AudioStreamer.h"

ClientManager::ClientManager(std::shared_ptr<AudioStreamer> streamer) : streamer(std::move(streamer)), clients(), pollFds(), clientMutex() {
    broadcastAudio = [&](Packet packet) {
        std::unique_lock<std::mutex> lock(clientMutex);

        if (poll(pollFds.data(), pollFds.size(), 100) <= 0) {
            return;
        }

        for (int i = 0; i < pollFds.size(); ++i) {
            if (pollFds[i].revents & POLLOUT) {
                clients[i]->sendPacketToClient(packet);
            }
        }
    };

    broadcastInfo = [&]() {
        std::unique_lock<std::mutex> lock(clientMutex);

        if (poll(pollFds.data(), pollFds.size(), 100) <= 0) {
            return;
        }

        Packet streamSize {};
        Packet streamName {};
        Packet streamHeader {};

        this->streamer->getCurrentFileSizePacket(streamSize);
        this->streamer->getCurrentFileNamePacket(streamName);
        this->streamer->getCurrentFileHeaderPacket(streamHeader);

        for (int i = 0; i < pollFds.size(); ++i) {
            if (pollFds[i].revents & POLLOUT) {
                clients[i]->sendPacketToClient(streamSize);
                clients[i]->sendPacketToClient(streamName);
                clients[i]->sendPacketToClient(streamHeader);
            }
        }

        freePacket(streamSize);
        freePacket(streamName);
        freePacket(streamHeader);
    };

    this->streamer->broadcastAudio = broadcastAudio;
    this->streamer->broadcastInfo = broadcastInfo;
}

std::shared_ptr<ClientManager> ClientManager::getPtr() {
    return shared_from_this();
}

void ClientManager::addClient(std::unique_ptr<Client>& newClient, pollfd newPollFd) {
    std::unique_lock<std::mutex> lock(clientMutex);

    sendInfoToClient(newClient);

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    clients.push_back(std::move(newClient));
    pollFds.push_back(newPollFd);
    anyClients = true;

    std::cout << "Client connected\n";
}

void ClientManager::removeClient(int clientSocket) {
    auto it = std::find_if(clients.begin(), clients.end(),
                           [clientSocket](const auto& c) { return c->getSocket() == clientSocket; });
    if (it != clients.end()) {
        clients.erase(it);
    }

    auto pollIt = std::find_if(pollFds.begin(), pollFds.end(), [clientSocket](const auto& p) { return p.fd == clientSocket; });
    if (pollIt != pollFds.end()) {
        pollFds.erase(pollIt);
    }

    if (pollFds.empty()) {
        anyClients = false;
    }

    std::cout << "Client disconnected\n";
}

void ClientManager::handleRequests() {
    while (!closeServer) {
        if (!anyClients) {
            continue;
        }

        int pollResult = poll(pollFds.data(), pollFds.size(), 10000);

        if (pollResult == -1) {
            closeServer = true;
            break;
        }
        {
            std::unique_lock<std::mutex> lock(clientMutex);

            for (int i = 0; i < pollFds.size(); i++) {
                if (pollFds[i].revents & POLLIN) {
                    Packet packet {};
                    clients[i]->receivePacketFromClient(packet);

                    if (packet.packetType == PacketType::END) {
                        continue;
                    }

                    if (packet.packetType == PacketType::SKIP) {
                        streamer->isRequestedNext = true;
                    } else if (packet.packetType == PacketType::REMOVE) {
                        streamer->removeFromQueue(parseRequest(packet)[0]);
                    } else if (packet.packetType == PacketType::ADD) {
                        streamer->addToQueue(parseRequest(packet)[0]);
                    } else if (packet.packetType == PacketType::REORDER) {
                        streamer->reorderQueue(parseRequest(packet));
                    }

                    freePacket(packet);
                }
                if (pollFds[i].revents & (POLLHUP | POLLERR)) {
                    clients.erase(clients.begin() + i);
                    pollFds.erase(pollFds.begin() + i);
                }
            }
        }

    }
}

void ClientManager::sendInfoToClient(std::unique_ptr<Client>& client) {
    Packet streamSize {};
    Packet streamName {};
    Packet streamHeader {};

    streamer->getCurrentFileSizePacket(streamSize);
    streamer->getCurrentFileNamePacket(streamName);
    streamer->getCurrentFileHeaderPacket(streamHeader);

    client->sendPacketToClient(streamSize);
    client->sendPacketToClient(streamName);
    client->sendPacketToClient(streamHeader);

    freePacket(streamSize);
    freePacket(streamName);
    freePacket(streamHeader);
}

void ClientManager::closeClients() {
    std::unique_lock<std::mutex> lock(clientMutex);
    clients.clear();
    pollFds.clear();
}
