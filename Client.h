#include <vector>
#include <ios>
#include <filesystem>
#include <utility>
#include <sys/socket.h>
#include <memory_resource>
#include <cstring>
#include "Packet.h"


#ifndef INTERNET_RADIO_CLIENT_H
#define INTERNET_RADIO_CLIENT_H

class ClientManager;

class Client : std::enable_shared_from_this<Client> {
private:
    const int socket;
    std::shared_ptr<ClientManager> manager;

    void handleError();
public:
    explicit Client(std::shared_ptr<ClientManager> manager, int socket);

    virtual ~Client();

    int getSocket() const;

    void sendPacketToClient(Packet& packet);

    void receivePacketFromClient(Packet& packet);
};


#endif //INTERNET_RADIO_CLIENT_H
