#include <stdio.h>
#include <string.h>
#include <SFML/Network.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "resources.h"
#include "soundManager.h"

#define MAX_SOUNDS 16

SoundManager soundManager;

SoundManager::SoundManager()
{
    //By creating a SoundBuffer we force SFML to load the sound subsystem. Else this is done when the first sound is loaded, causing a delay.
    sf::SoundBuffer forceLoadBuffer;

    for(unsigned int n=0; n<MAX_SOUNDS; n++)
        activeSoundList.push_back(sf::Sound());
    
    musicStream = NULL;
}

SoundManager::~SoundManager()
{
}

void SoundManager::playMusic(string name)
{
    music.stop();
    musicStream = getResourceStream(name);
    if (musicStream)
    {
        music.openFromStream(**musicStream);
        music.play();
    }
}

void SoundManager::stopMusic()
{
    music.stop();
}

void SoundManager::playSound(string name, float pitch, float volume)
{
    sf::SoundBuffer* data = soundMap[name];
    if (data == NULL)
        data = loadSound(name);
    
    playSoundData(data, pitch, volume);
}

void SoundManager::setTextToSpeachVoice(string name)
{
}

string url_encode(const string &value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        string::value_type c = (*i);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        }
        else if (c == ' ')  {
            escaped << '+';
        }
        else {
            escaped << '%' << std::setw(2) << ((int) c) << std::setw(0);
        }
    }

    return escaped.str();
}

void SoundManager::playTextToSpeech(string text)
{
    string name = "TTS:" + text;
    sf::SoundBuffer* data = soundMap[name];
    if (data != NULL)
    {
        playSoundData(data, 1.0, 100.0);
        return;
    }

    sf::Http http("localhost", 59125);
    sf::Http::Request request("process?INPUT_TEXT=" + url_encode(text) + "&INPUT_TYPE=TEXT&OUTPUT_TYPE=AUDIO&AUDIO=WAVE_FILE&LOCALE=en_US&VOICE=dfki-prudence");
    sf::Http::Response response = http.sendRequest(request);
    
    sf::Http::Response::Status status = response.getStatus();
    if (status == sf::Http::Response::Ok)
    {
        string wave = response.getBody();
        sf::SoundBuffer* data = new sf::SoundBuffer();
        data->loadFromMemory(wave.data(), wave.size());
        soundMap[name] = data;
        playSoundData(data, 1.0, 100.0);
    }
    else
    {
        std::cout << "Error requesting text to speech from Mary server: " << status << std::endl;
    }
}

void SoundManager::playSoundData(sf::SoundBuffer* data, float pitch, float volume)
{
    for(unsigned int n=0; n<activeSoundList.size(); n++)
    {
        sf::Sound& sound = activeSoundList[n];
        if (sound.getStatus() == sf::Sound::Stopped)
        {
            sound.setBuffer(*data);
            sound.setPitch(pitch);
            sound.setVolume(volume);
            sound.play();
            return;
        }
    }
}

sf::SoundBuffer* SoundManager::loadSound(string name)
{
    sf::SoundBuffer* data = soundMap[name];
    if (data)
        return data;
    
    data = new sf::SoundBuffer();
    
    P<ResourceStream> stream = getResourceStream(name);
    if (!stream) stream = getResourceStream(name + ".wav");
    if (!stream || !data->loadFromStream(**stream))
    {
        printf("Failed to load: %s\n", name.c_str());
        soundMap[name] = data;
        return data;
    }
    
    printf("Loaded: %s of %f seconds\n", name.c_str(), data->getDuration().asSeconds());
    soundMap[name] = data;
    return data;
}
