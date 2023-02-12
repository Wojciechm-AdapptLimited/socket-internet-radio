#include "MusicPlayer.h"

MusicPlayer::MusicPlayer() {
    music = nullptr;
    audioData = nullptr;
    audioDataSize = 0;
    audioDataCurPos = 0;
    audioDataCreated = false;
}

void MusicPlayer::setSongName(std::string songName) {
    audioName = songName;
}

void MusicPlayer::startFetchAudio(int musicSize) {
    audioDataCreated = false;
    audioDataSize = musicSize;
    audioData = std::make_unique<irrklang::ik_c8[]>(audioDataSize);
    audioDataCurPos = 0;
}

bool MusicPlayer::readyToPlayMusic() const {
    return audioDataCurPos > 500000;
}

void MusicPlayer::fetchAudioToMemory(char* musicData, int musicDataSize) {
    std::memcpy(audioData.get() + audioDataCurPos, musicData, musicDataSize);
    audioDataCurPos += musicDataSize;

    if(!audioDataCreated)
    {
        s_engine->addSoundSourceFromMemory(audioData.get(), audioDataSize, audioName.c_str(), false);
        audioDataCreated = true;
    }
}

void MusicPlayer::playMusic() {
    music = s_engine->play2D(audioName.c_str(), true, false, false, irrklang::ESM_STREAMING, true);
}

void MusicPlayer::stopMusic() {
    if(music)
    {
        s_engine->removeSoundSource(audioName.c_str());
        music->drop();
    }
}

void initEngine() {
    s_engine = irrklang::createIrrKlangDevice();
}

void shutdownEngine() {
    s_engine->drop();
}
