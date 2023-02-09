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

void Client::sendAudio(const std::pmr::vector<char>& audioBuffer, std::streamsize audioSize) {
    char header = 0x01;
    std::size_t sent = send(socket, &header, 1, 0);
    if (sent == -1) {
        handleError();
        return;
    }

    usleep(500);

    sent = send(socket, audioBuffer.data(), audioSize, 0);
    if (sent == -1) {
        handleError();
        return;
    }
}

void Client::sendFileList(const std::string& message) {
    char header = 0x02;
    std::size_t sent = send(socket, &header, 1, 0);
    if (sent == -1) {
        handleError();
        return;
    }

    sent = send(socket, message.c_str(), message.size(), 0);
    if (sent == -1) {
        handleError();
        return;
    }
}

std::string Client::readRequest() {
    std::vector<char> requestBuffer(BUFFER_SIZE);

    std::size_t received = recv(socket, requestBuffer.data(), BUFFER_SIZE, 0);

    if (received == -1) {
        handleError();
        return {};
    }

    return {requestBuffer.data(), received};
}

std::optional<Request> Client::receiveRequest() {
    std::string requestString = readRequest();
    auto request = parseRequest(requestString);

    if (!request.has_value()) {
        return {};
    }

    return request;
}