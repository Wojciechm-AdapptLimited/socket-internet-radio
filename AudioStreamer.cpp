#include <algorithm>
#include "AudioStreamer.h"


std::optional<std::filesystem::path> AudioStreamer::getCurrentFileName() {
    std::unique_lock<std::mutex> lock(audioMutex);

    if (audioQueue.empty()) {
        return {};
    }

    std::filesystem::path currentFile = audioQueue.front();
    audioQueue.pop_front();
    return currentFile;
}

void AudioStreamer::streamAudioFile(std::ifstream& audio) {
    std::pmr::vector<char> audioBuffer(DATA_SIZE, 0, &memPool);

    audio.seekg (0, std::ifstream::end);
    currentFileSize = audio.tellg();
    audio.seekg (0, std::ifstream::beg);

    int sent = audio.readsome(currentFileHeader.data(), DATA_SIZE);

    broadcastInfo();

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    while (sent < currentFileSize && !finishStreaming) {
        if (isRequestedNext) {
            isRequestedNext = false;
            return;
        }

        int read = audio.readsome(audioBuffer.data(), DATA_SIZE);

        Packet musicStream {};
        musicStream.packetType = PacketType::STREAM;
        musicStream.dataSize = read;
        musicStream.data = new char[DATA_SIZE];
        memcpy(musicStream.data, audioBuffer.data(), read);

        broadcastAudio(musicStream);

        freePacket(musicStream);

        sent += read;

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void AudioStreamer::readDirectory(const std::filesystem::path& dir) {
    std::unique_lock<std::mutex> lock(audioMutex);

    for (const auto& entry: std::filesystem::directory_iterator(dir)) {
        if (std::filesystem::is_regular_file(entry.path()) && entry.path().extension() == ".wav") {
            audioQueue.emplace_back(entry.path());
            audioFiles.push_back(entry.path());
        }
    }
}

void AudioStreamer::streamAudioQueue() {
    while (!finishStreaming) {
        std::optional<std::filesystem::path> currentFile = getCurrentFileName();

        if (!currentFile.has_value()) {
            std::cerr << "There are no available files to play, ending streaming...\n";
            return;
        }

        currentFileName = currentFile.value();

        std::ifstream audio(currentFileName, std::ios::binary);

        if (!audio.is_open()) {
            std::cerr << "Error opening file: " << currentFileName << "\n";
            continue;
        }

        streamAudioFile(audio);

        audio.close();

        {
            std::unique_lock<std::mutex> lock(audioMutex);
            audioQueue.emplace_back(currentFileName);
        }
    }
}

void AudioStreamer::reorderQueue(const std::vector<std::filesystem::path>& newOrder) {
    std::unique_lock<std::mutex> lock(audioMutex);

    audioQueue.clear();
    std::copy_if(newOrder.begin(), newOrder.end(), std::back_inserter(audioQueue), [this](const std::filesystem::path& path) {
        return std::ranges::any_of(audioFiles, [path](const std::filesystem::path& file) {
            return file.filename() == path;
        });
    });
}

void AudioStreamer::addToQueue(const std::filesystem::path& path) {
    std::unique_lock<std::mutex> lock(audioMutex);\

    auto it = std::ranges::find_if(audioFiles, [path](const std::filesystem::path& file) {
        return file.filename() == path;
    });

    if (it != audioFiles.end()) {
        audioQueue.emplace_back(*it);
    }
}

void AudioStreamer::removeFromQueue(const std::filesystem::path& path) {
    std::unique_lock<std::mutex> lock(audioMutex);

    std::erase_if(audioQueue, [path](const std::filesystem::path& p) {
        return p.filename() == path;
    });
}

void AudioStreamer::getAvailableFilesPacket(Packet& packet) {
    std::unique_lock<std::mutex> lock(audioMutex);

    std::string message;
    for(const auto& file : audioFiles){
        message += file.filename().string() + ";";
    }

    packet.packetType = PacketType::FILES;
    packet.dataSize = static_cast<unsigned int>(message.size() + 1);
    packet.data = new char[DATA_SIZE];
    memcpy(packet.data, message.data(), packet.dataSize);
}

void AudioStreamer::getQueueStatePacket(Packet& packet) {
    std::unique_lock<std::mutex> lock(audioMutex);

    std::string message;
    for(const auto& file : audioQueue){
        message += file.filename().string() + ";";
    }

    packet.packetType = PacketType::QUEUE;
    packet.dataSize = static_cast<unsigned int>(message.size() + 1);
    packet.data = new char[DATA_SIZE];
    memcpy(packet.data, message.data(), packet.dataSize);
}

void AudioStreamer::getCurrentFileSizePacket(Packet& packet) {
    std::unique_lock<std::mutex> lock(audioMutex);

    packet.packetType = PacketType::STREAM_SIZE;
    packet.dataSize = sizeof(int);
    packet.data = new char[DATA_SIZE];
    memcpy(packet.data, &currentFileSize, packet.dataSize);
}

void AudioStreamer::getCurrentFileNamePacket(Packet& packet) {
    std::unique_lock<std::mutex> lock(audioMutex);

    std::string fileName = currentFileName.filename().string();
    packet.packetType = PacketType::STREAM_NAME;
    packet.dataSize = static_cast<unsigned int>(fileName.size() + 1);
    packet.data = new char[DATA_SIZE];
    memcpy(packet.data, fileName.data(), packet.dataSize);
}

void AudioStreamer::getCurrentFileHeaderPacket(Packet& packet) {
    std::unique_lock<std::mutex> lock(audioMutex);

    packet.packetType = PacketType::STREAM_HEADER;
    packet.dataSize = currentFileHeader.size();
    packet.data = new char[DATA_SIZE];
    memcpy(packet.data, currentFileHeader.data(), packet.dataSize);
}
