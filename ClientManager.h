#include <vector>
#include <mutex>
#include <memory>
#include <sys/poll.h>
#include <functional>
#include <atomic>
#include "Client.h"

#ifndef INTERNET_RADIO_CLIENTMANAGER_H
#define INTERNET_RADIO_CLIENTMANAGER_H

class AudioStreamer;

class ClientManager : std::enable_shared_from_this<ClientManager> {
private:
    std::vector<std::unique_ptr<Client>> clients;
    std::vector<pollfd> pollFds;
    std::mutex clientMutex;
    std::shared_ptr<AudioStreamer> streamer;
public:
    std::atomic<bool> closeServer{false};
    std::function<void(const std::pmr::vector<char>& audioBuffer, std::streamsize audioSize)> streamAudioBuffer;

    explicit ClientManager(std::shared_ptr<AudioStreamer> streamer);

    void addClient(int socket);

    void removeClient(std::unique_ptr<Client> client);

    void handleRequests();
};


#endif //INTERNET_RADIO_CLIENTMANAGER_H
