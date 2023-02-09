#include "Request.h"

Request::Request(Request::Type type, const std::vector<std::filesystem::path>& files) : type(type), files(files) {}

const std::vector<std::filesystem::path>& Request::getFiles() const { return files; }

Request::Type Request::getType() const { return type; }

std::optional<Request> parseRequest(const std::string& requestString) {
    auto parts = split(requestString, ":");
    if (parts.size() != 2) {
        return {};
    }

    // Get the request type
    Request::Type type;
    if (parts[0] == "SKIP") {
        type = Request::Type::SKIP;
    } else if (parts[0] == "ADD") {
        type = Request::Type::ADD;
    } else if (parts[0] == "REMOVE") {
        type = Request::Type::REMOVE;
    } else if (parts[0] == "REORDER") {
        type = Request::Type::REORDER;
    } else {
        return {};
    }

    // Deserialize the vector of files
    std::vector<std::filesystem::path> files;
    if (type != Request::Type::SKIP) {
        std::vector<std::string> fileNamesStr = split(parts[1], ";");
        std::transform(fileNamesStr.begin(), fileNamesStr.end(), std::back_inserter(files), [&](const std::string& s) {
            return std::filesystem::path{s};
        });
    }

    return Request(type, files);
}

std::vector<std::string> split(std::string s, const std::string& delimiter) {
    size_t pos = 0;
    std::vector<std::string> vec;

    while ((pos = s.find(delimiter)) != std::string::npos) {
        vec.push_back(s.substr(0, pos));
        s.erase(0, pos + delimiter.length());
    }
    vec.push_back(s);

    return vec;
}
