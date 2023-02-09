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

    while (audio) {
        if (isRequestedNext) {
            isRequestedNext = false;
            return;
        }

        audio.read(audioBuffer.data(), BUFFER_SIZE);
        streamAudioBuffer(audioBuffer, audio.gcount());
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

        std::ifstream audio(currentFile.value(), std::ios::binary);

        if (!audio.is_open()) {
            std::cerr << "Error opening file: " << currentFile.value() << "\n";
            continue;
        }

        streamAudioFile(audio);

        audio.close();

        {
            std::unique_lock<std::mutex> lock(audioMutex);
            audioQueue.emplace_back(currentFile.value());
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

std::string AudioStreamer::getAvailableFilesSerialized() {
    std::unique_lock<std::mutex> lock(audioMutex);

    std::string message = "AVAILABLE_FILES:";
    for(const auto& file : audioFiles){
        message += file.string() + ";";
    }

    return message;
}

std::string AudioStreamer::getQueueStateSerialized() {
    std::unique_lock<std::mutex> lock(audioMutex);

    std::string message = "QUEUE_STATE:";
    for(const auto& file : audioQueue){
        message += file.string() + ";";
    }

    return message;
}

