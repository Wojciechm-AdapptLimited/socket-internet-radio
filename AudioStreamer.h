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
#include <cstring>

#include "Packet.h"


#ifndef INTERNET_RADIO_AUDIOSTREAMER_H
#define INTERNET_RADIO_AUDIOSTREAMER_H

class ClientManager;

class AudioStreamer {
private:
    std::deque<std::filesystem::path> audioQueue;
    std::vector<std::filesystem::path> audioFiles;

    std::vector<char> currentFileHeader = std::vector<char>(DATA_SIZE, 0);
    std::filesystem::path currentFileName;
    int currentFileSize;

    std::mutex audioMutex;
    std::pmr::monotonic_buffer_resource memPool;

    std::optional<std::filesystem::path> getCurrentFileName();

    void streamAudioFile(std::ifstream& audio);

public:
    std::atomic<bool> finishStreaming{false};
    std::atomic<bool> isRequestedNext{false};

    std::function<void(Packet& packet)> broadcastAudio;
    std::function<void()> broadcastInfo;

    void getAvailableFilesPacket(Packet& packet);

    void getQueueStatePacket(Packet& packet);

    void getCurrentFileSizePacket(Packet& packet);

    void getCurrentFileNamePacket(Packet& packet);

    void getCurrentFileHeaderPacket(Packet& packet);

    void readDirectory(const std::filesystem::path& dir);

    void reorderQueue(const std::vector<std::filesystem::path>& newOrder);

    void addToQueue(const std::filesystem::path& path);

    void removeFromQueue(const std::filesystem::path& path);

    void streamAudioQueue();
};


#endif //INTERNET_RADIO_AUDIOSTREAMER_H
