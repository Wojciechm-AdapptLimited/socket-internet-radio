#include "ClientManager.h"
#include "AudioStreamer.h"

ClientManager::ClientManager(std::shared_ptr<AudioStreamer> streamer) : streamer(std::move(streamer)), clients(), pollFds(), clientMutex() {
    streamAudioBuffer = [&](const std::pmr::vector<char>& audioBuffer, std::streamsize audioSize) {
        std::unique_lock<std::mutex> lock(clientMutex);

        if (poll(pollFds.data(), pollFds.size(), 100) < 0) {
            return;
        }

        for (int i = 0; i < pollFds.size(); ++i) {
            if (pollFds[i].revents & POLLOUT) {
                clients[i]->sendAudio(audioBuffer, audioSize);
            }
        }
    };
    this->streamer->streamAudioBuffer = streamAudioBuffer;
}

std::shared_ptr<ClientManager> ClientManager::getPtr() {
    return shared_from_this();
}

void ClientManager::addClient(std::unique_ptr<Client>& newClient, pollfd newPollFd) {
    std::unique_lock<std::mutex> lock(clientMutex);

    //newClient->sendFileList(streamer->getAvailableFilesSerialized());
    //newClient->sendFileList(streamer->getQueueStateSerialized());

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
                    std::optional<Request> request = clients[i]->receiveRequest();

                    if (!request.has_value()) {
                        continue;
                    }

                    Request::Type requestType = request.value().getType();

                    if (requestType == Request::Type::SKIP) {
                        streamer->isRequestedNext = true;
                    } else if (requestType == Request::Type::REMOVE) {
                        streamer->removeFromQueue(request.value().getFiles()[0]);
                    } else if (requestType == Request::Type::ADD) {
                        streamer->addToQueue(request.value().getFiles()[0]);
                    } else if (requestType == Request::Type::REORDER) {
                        streamer->reorderQueue(request.value().getFiles());
                    }

                    std::string allFiles = streamer->getAvailableFilesSerialized();
                    std::string queue = streamer->getQueueStateSerialized();

                    for (const auto& client : clients) {
                        client->sendFileList(allFiles);
                        client->sendFileList(queue);
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
