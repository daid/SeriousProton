#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include "audio/music.h"
#include "audio/sound.h"

#include <unordered_map>
#include <vector>
#include <array>
#include "timer.h"
#include "resources.h"
#include "stringImproved.h"

class SoundManager;
extern SoundManager* soundManager;
class SoundManager
{
private:
    static constexpr float fade_music_time = 1.0f;
    static constexpr float fade_sound_time = 0.3f;

    enum FadeMode
    {
        None,
        FadeIn,
        FadeOut
    };
    struct MusicChannel
    {
        string next_stream;
        sp::audio::Music music;
        FadeMode mode;
        float fade_delay;
    };
    struct SoundChannel
    {
        bool positional = false;
        float min_distance = 1.0f;
        float attenuation = 30.0f;
        float volume = 1.0f;
        glm::vec2 position;
        sp::audio::SoundPlayback playback;
    };
    sp::SystemStopwatch clock;
    MusicChannel music_channel;

    std::vector<string> music_set;

    std::unordered_map<string, sp::audio::Sound*> sound_map;
    std::array<SoundChannel, 16> active_sound_list;
    float music_volume;
    float master_sound_volume;

    bool positional_sound_enabled;
    glm::vec2 listener_position;
public:
    SoundManager();
    ~SoundManager();

    // Music
    void playMusic(string filename);
    void playMusicSet(std::vector<string> filenames);
    void stopMusic();
    void setMusicVolume(float volume);
    float getMusicVolume();

    // Non-positional sounds
    int playSound(string name, float pitch = 1.0f, float volume = 100.0f, bool loop = false);

    // Positional sounds
    int playSound(string name, glm::vec2 position, float min_distance, float attenuation, float pitch = 1.0f, float volume = 100.0f, bool loop = false);
    void setListenerPosition(glm::vec2 position, float angle);
    void disablePositionalSound();

    // Sound management
    void stopSound(int index);
    void setMasterSoundVolume(float volume); // Valid values 0.0f-100.0f
    float getMasterSoundVolume();
    void setSoundVolume(int index, float volume); // Valid values 0.0f-100.0f
    void setSoundPitch(int index, float volume); // Valid values 0.0f+; 1.0f = default

private:
    int playSoundData(sp::audio::Sound* data, float pitch, float volume, bool loop = false);
    sp::audio::Sound* loadSound(const string& name);
    void updateChannelVolume(SoundChannel& channel);

    void startMusic(const string& name, bool loop=false);

    void updateTick();
    void updateChannel(MusicChannel& channel, float delta);

    friend class Engine;
};

#endif//SOUNDMANAGER_H
