#include "ClientManager.h"
#include "AudioStreamer.h"

ClientManager::ClientManager(std::shared_ptr<AudioStreamer> streamer) : streamer(std::move(streamer)) {
    this->streamer->streamAudioBuffer = streamAudioBuffer;
    streamAudioBuffer = [&](const std::pmr::vector<char>& audioBuffer, std::streamsize audioSize) {
        std::unique_lock<std::mutex> lock(clientMutex);

        for (const auto& client: clients) {
            client->sendAudio(audioBuffer, audioSize);
        }
    };
}

void ClientManager::addClient(int socket) {
    std::unique_lock<std::mutex> lock(clientMutex);

    auto newClient = std::make_unique<Client>(shared_from_this(), socket);
    pollfd newPollFd = {socket, POLLIN, 0};

    clients.push_back(std::move(newClient));
    pollFds.push_back(newPollFd);
}

void ClientManager::removeClient(std::unique_ptr<Client> client) {
    std::unique_lock<std::mutex> lock(clientMutex);

    int socket = client->getSocket();
    auto it = std::find_if(clients.begin(), clients.end(),
                           [socket](const auto& c) { return c->getSocket() == socket; });
    if (it != clients.end()) {
        clients.erase(it);
    }

    auto pollIt = std::find_if(pollFds.begin(), pollFds.end(), [socket](const auto& p) { return p.fd == socket; });
    if (pollIt != pollFds.end()) {
        pollFds.erase(pollIt);
    }
}

void ClientManager::handleRequests() {
    while (true) {

    }
}
