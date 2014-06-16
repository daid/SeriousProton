#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <SFML/Audio.hpp>

#include <map>
#include <vector>
#include "resources.h"
#include "stringImproved.h"

class SoundManager;
extern SoundManager soundManager;
class SoundManager
{
private:
    sf::Music music;
    P<ResourceStream> musicStream;
    std::map<string, sf::SoundBuffer*> soundMap;
    std::vector<sf::Sound> activeSoundList;
public:
    SoundManager();
    ~SoundManager();
    
    void playMusic(string name);
    void stopMusic();
    void playSound(string name, float pitch = 1.0f, float volume = 100.0f);
    void setTextToSpeachVoice(string name);
    void playTextToSpeech(string text);
private:
    void playSoundData(sf::SoundBuffer* data, float pitch, float volume);
    sf::SoundBuffer* loadSound(string name);
};

#endif//SOUNDMANAGER_H
