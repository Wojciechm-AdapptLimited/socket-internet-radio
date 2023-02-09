#include <deque>
#include <vector>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <filesystem>
#include <memory_resource>
#include <optional>
#include <functional>


#ifndef INTERNET_RADIO_AUDIOSTREAMER_H
#define INTERNET_RADIO_AUDIOSTREAMER_H

class ClientManager;

class AudioStreamer {
private:
    std::deque<std::filesystem::path> audioQueue;
    std::vector<std::filesystem::path> audioFiles;
    std::mutex audioMutex;
    std::pmr::monotonic_buffer_resource memPool;

    std::optional<std::filesystem::path> getCurrentFileName();

    void streamAudioFile(std::ifstream& audio);

public:
    static constexpr std::size_t BUFFER_SIZE = 1024;

    std::atomic<bool> finishStreaming{false};
    std::atomic<bool> isRequestedNext{false};

    std::function<void(const std::pmr::vector<char>& audioBuffer, std::streamsize audioSize)> streamAudioBuffer;

    std::string getAvailableFilesSerialized();

    std::string getQueueStateSerialized();

    void readDirectory(const std::filesystem::path& dir);

    void reorderQueue(const std::vector<std::filesystem::path>& newOrder);

    void addToQueue(const std::filesystem::path& path);

    void removeFromQueue(const std::filesystem::path& path);

    void streamAudioQueue();
};


#endif //INTERNET_RADIO_AUDIOSTREAMER_H
