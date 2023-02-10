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
    std::pmr::vector<char> audioBuffer(BUFFER_SIZE, 0, &memPool);

    audio.seekg (0, std::ifstream::end);
    currentFileSize = audio.tellg();
    audio.seekg (0, std::ifstream::beg);

    Packet packet = getCurrentFileSizePacket();
    broadcastPacket(packet);

    while (audio) {
        if (isRequestedNext) {
            isRequestedNext = false;
            return;
        }

        audio.read(audioBuffer.data(), BUFFER_SIZE);

        Packet musicStream {};
        musicStream.packetType = PacketType::STREAM;
        musicStream.dataSize = audio.gcount();
        musicStream.data = audioBuffer.data();

        broadcastPacket(musicStream);
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
    while (true) {
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

        if (finishStreaming) {
            return;
        }
    }
}

void AudioStreamer::reorderQueue(const std::vector<std::filesystem::path>& newOrder) {
    std::unique_lock<std::mutex> lock(audioMutex);

    audioQueue.clear();
    std::copy_if(newOrder.begin(), newOrder.end(), std::back_inserter(audioQueue), [this](const auto& path) {
        return std::find(audioFiles.begin(), audioFiles.end(), path) != audioFiles.end();
    });
}

void AudioStreamer::addToQueue(const std::filesystem::path& path) {
    std::unique_lock<std::mutex> lock(audioMutex);

    if (std::find(audioFiles.begin(), audioFiles.end(), path) != audioFiles.end()) {
        audioQueue.emplace_back(path);
    }
}

void AudioStreamer::removeFromQueue(const std::filesystem::path& path) {
    std::unique_lock<std::mutex> lock(audioMutex);

    audioQueue.erase(std::remove(audioQueue.begin(), audioQueue.end(), path), audioQueue.end());
}

Packet AudioStreamer::getAvailableFilesPacket() {
    std::unique_lock<std::mutex> lock(audioMutex);

    std::string message;
    for(const auto& file : audioFiles){
        message += file.string() + ";";
    }

    Packet allFilesPacket {};
    allFilesPacket.packetType = PacketType::FILES;
    allFilesPacket.dataSize = sizeof(message.data());
    allFilesPacket.data = message.data();

    return allFilesPacket;
}

Packet AudioStreamer::getQueueStatePacket() {
    std::unique_lock<std::mutex> lock(audioMutex);

    std::string message;
    for(const auto& file : audioQueue){
        message += file.string() + ";";
    }

    Packet queuePacket {};
    queuePacket.packetType = PacketType::QUEUE;
    queuePacket.dataSize = sizeof(message.data());
    queuePacket.data = message.data();

    return queuePacket;
}

Packet AudioStreamer::getCurrentFileSizePacket() {
    Packet musicStreamSize {};
    musicStreamSize.packetType = PacketType::SIZE;
    musicStreamSize.dataSize = sizeof(long);
    musicStreamSize.data = new char[DATA_SIZE];
    std::memcpy(musicStreamSize.data, &currentFileSize, sizeof(long));

    return musicStreamSize;
}