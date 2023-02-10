#include <string>
#include <cstring>
#include <memory>
#include <irrKlang.h>


#ifndef INTERNET_RADIO_MUSICPLAYER_H
#define INTERNET_RADIO_MUSICPLAYER_H

static irrklang::ISoundEngine* s_engine;

class MusicPlayer {
public:
    explicit MusicPlayer();
    void setSongName(const char* songName);
    void startFetchAudio(int musicSize);
    void fetchAudioToMemory(char* musicData, unsigned int musicDataSize);

    [[nodiscard]] bool readyToPlayMusic() const;
    void playMusic();
    void stopMusic();


private:
    irrklang::ISound* music;
    std::unique_ptr<irrklang::ik_c8[]> audioData;
    irrklang::ik_s32 audioDataSize;
    irrklang::ik_s32 audioDataCurPos;
    bool audioDataCreated;
    std::string audioName;
};

void initEngine();
void shutdownEngine();

#endif //INTERNET_RADIO_MUSICPLAYER_H
