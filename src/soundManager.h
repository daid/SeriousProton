#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <SFML/Audio.hpp>

#include <unordered_map>
#include <vector>
#include "resources.h"
#include "stringImproved.h"

class SoundManager;
extern SoundManager* soundManager;
class SoundManager
{
private:
    static constexpr float fade_music_time = 1.0;
    static constexpr float fade_sound_time = 0.3;

    enum FadeMode
    {
        None,
        FadeIn,
        FadeOut
    };
    struct MusicChannel
    {
        P<ResourceStream> stream;
        P<ResourceStream> next_stream;
        sf::Music music;
        FadeMode mode;
        float fade_delay;
    };
    sf::Clock clock;
    MusicChannel music_channel;

    std::vector<string> music_set;

    std::unordered_map<string, sf::SoundBuffer*> soundMap;
    std::vector<sf::Sound> activeSoundList;
    float music_volume;
    float master_sound_volume;
    bool positional_sound_enabled;
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
    int playSound(string name, sf::Vector2f position, float min_distance, float attenuation, float pitch = 1.0f, float volume = 100.0f, bool loop = false);
    void setListenerPosition(sf::Vector2f position, float angle);
    void disablePositionalSound();

    // Sound management
    void stopSound(int index);
    void setMasterSoundVolume(float volume); // Valid values 0.0f-100.0f
    float getMasterSoundVolume();
    void setSoundVolume(int index, float volume); // Valid values 0.0f-100.0f
    float getSoundVolume(int index);
    void setSoundPitch(int index, float volume); // Valid values 0.0f+; 1.0 = default
    float getSoundPitch(int index);

    // TTS
    void setTextToSpeachVoice(string name);
    void playTextToSpeech(string text);
private:
    int playSoundData(sf::SoundBuffer* data, float pitch, float volume, bool loop = false);
    sf::SoundBuffer* loadSound(string name);

    void startMusic(P<ResourceStream> stream, bool loop=false);

    void updateTick();
    void updateChannel(MusicChannel& channel, float delta);

    friend class Engine;
};

#endif//SOUNDMANAGER_H
