#include "ClientManager.h"
#include "AudioStreamer.h"

ClientManager::ClientManager(std::shared_ptr<AudioStreamer> streamer) : streamer(std::move(streamer)), clients(), pollFds(), clientMutex() {
    broadcastPacket = [&](Packet packet) {
        std::unique_lock<std::mutex> lock(clientMutex);

        if (poll(pollFds.data(), pollFds.size(), 100) < 0) {
            return;
        }

        for (int i = 0; i < pollFds.size(); ++i) {
            if (pollFds[i].revents & POLLOUT) {
                clients[i]->sendPacketToClient(packet);
            }
        }
    };

    this->streamer->broadcastPacket = broadcastPacket;
}

std::shared_ptr<ClientManager> ClientManager::getPtr() {
    return shared_from_this();
}

void ClientManager::addClient(std::unique_ptr<Client>& newClient, pollfd newPollFd) {
    std::unique_lock<std::mutex> lock(clientMutex);

    //newClient->sendFileList(streamer->getAvailableFilesSerialized());
    //newClient->sendFileList(streamer->getQueueStateSerialized());
    Packet packet = streamer->getCurrentFileSizePacket();
    newClient->sendPacketToClient(packet);

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
                    std::optional<Packet> request = clients[i]->receivePacketFromClient();

                    if (!request.has_value()) {
                        continue;
                    }

                    PacketType requestType = request.value().packetType;

                    if (requestType == PacketType::SKIP) {
                        streamer->isRequestedNext = true;
                    } else if (requestType == PacketType::REMOVE) {
                        streamer->removeFromQueue(parseRequest(request.value())[0]);
                    } else if (requestType == PacketType::ADD) {
                        streamer->addToQueue(parseRequest(request.value())[0]);
                    } else if (requestType == PacketType::REORDER) {
                        streamer->reorderQueue(parseRequest(request.value()));
                    }

                    Packet allFiles = streamer->getAvailableFilesPacket();
                    Packet queue = streamer->getQueueStatePacket();

                    for (const auto& client : clients) {
                        client->sendPacketToClient(allFiles);
                        client->sendPacketToClient(queue);
                    }
                }
                if (pollFds[i].revents & (POLLHUP | POLLERR)) {
                    clients.erase(clients.begin() + i);
                    pollFds.erase(pollFds.begin() + i);
                }
            }
        }

    }
}

void ClientManager::closeClients() {
    std::unique_lock<std::mutex> lock(clientMutex);
    clients.clear();
    pollFds.clear();
}
