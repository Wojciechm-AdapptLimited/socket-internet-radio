#include <vector>
#include <mutex>
#include <memory>
#include <sys/poll.h>
#include <functional>
#include <atomic>
#include <condition_variable>
#include "Client.h"

#ifndef INTERNET_RADIO_CLIENTMANAGER_H
#define INTERNET_RADIO_CLIENTMANAGER_H

class AudioStreamer;

class ClientManager : public std::enable_shared_from_this<ClientManager> {
private:
    std::vector<std::unique_ptr<Client>> clients;
    std::vector<pollfd> pollFds;
    std::mutex clientMutex;
    std::shared_ptr<AudioStreamer> streamer;
    std::atomic<bool> anyClients{false};
public:
    std::atomic<bool> closeServer{false};
    std::function<void(Packet& packet)> broadcastAudio;
    std::function<void()> broadcastInfo;

    explicit ClientManager(std::shared_ptr<AudioStreamer> streamer);

    std::shared_ptr<ClientManager> getPtr();

    void addClient(std::unique_ptr<Client>& newClient, pollfd newPollFd);

    void removeClient(int clientSocket);

    void handleRequests();

    void sendInfoToClient(std::unique_ptr<Client>& client);

    void closeClients();
};


#endif //INTERNET_RADIO_CLIENTMANAGER_H
