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
    static constexpr float fade_music_time = 10.0;

    enum FadeMode
    {
        None,
        FadeIn,
        FadeOut
    };
    struct MusicChannel
    {
        P<ResourceStream> stream;
        sf::Music music;
        FadeMode mode;
        float fade_delay;
    };
    sf::Clock clock;
    MusicChannel primary_music;
    MusicChannel secondary_music;
    
    std::vector<string> music_set;
    
    std::unordered_map<string, sf::SoundBuffer*> soundMap;
    std::vector<sf::Sound> activeSoundList;
    float music_volume;
    bool positional_sound_enabled;
public:
    SoundManager();
    ~SoundManager();
    
    void playMusic(string filename);
    void playMusicSet(std::vector<string> filenames);
    void stopMusic();
    void setMusicVolume(float volume);
    float getMusicVolume();
    void playSound(string name, float pitch = 1.0f, float volume = 100.0f);

    //Positional sounds
    void setListenerPosition(sf::Vector2f position, float angle);
    void disablePositionalSound();
    void playSound(string name, sf::Vector2f position, float min_distance, float attenuation, float pitch = 1.0f, float volume = 100.0f);

    void setTextToSpeachVoice(string name);
    void playTextToSpeech(string text);
private:
    void playSoundData(sf::SoundBuffer* data, float pitch, float volume);
    sf::SoundBuffer* loadSound(string name);
    
    void startMusic(P<ResourceStream> stream, bool loop=false);
    
    void updateTick();
    void updateChannel(MusicChannel& channel, float delta);
    
    friend class Engine;
};

#endif//SOUNDMANAGER_H
