#include <vector>
#include <ios>
#include <filesystem>
#include <utility>
#include <sys/socket.h>
#include <memory_resource>
#include "Request.h"


#ifndef INTERNET_RADIO_CLIENT_H
#define INTERNET_RADIO_CLIENT_H

class ClientManager;

class Client : std::enable_shared_from_this<Client> {
private:
    static constexpr int BUFFER_SIZE = 1024;
    const int socket;
    std::shared_ptr<ClientManager> manager;

    void handleError();

    std::string readRequest();
public:
    explicit Client(std::shared_ptr<ClientManager> manager, int socket);

    virtual ~Client();

    int getSocket() const;

    void sendAudio(const std::pmr::vector<char>& audioBuffer, std::streamsize audioSize);

    void sendFileList(const std::string& message);

    std::optional<Request> receiveRequest();
};


#endif //INTERNET_RADIO_CLIENT_H
