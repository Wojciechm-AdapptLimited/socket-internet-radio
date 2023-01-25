#include <filesystem>
#include <vector>
#include <optional>

#ifndef INTERNET_RADIO_REQUEST_H
#define INTERNET_RADIO_REQUEST_H

class Request {
public:
    enum class Type {
        SKIP, ADD, REMOVE, REORDER
    };

    Request(Type type, const std::vector<std::filesystem::path>& files);

    [[nodiscard]] Type getType() const;
    [[nodiscard]] const std::vector<std::filesystem::path>& getFiles() const;

private:
    Type type;
    std::vector<std::filesystem::path> files;
};

std::vector<std::string> split(std::string s, const std::string& delimiter);
std::optional<Request> parseRequest(const std::string& requestString);

#endif //INTERNET_RADIO_REQUEST_H
