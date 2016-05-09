#include <stdio.h>
#include <string.h>
#include <SFML/Network.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "vectorUtils.h"
#include "resources.h"
#include "soundManager.h"
#include "random.h"

#define MAX_SOUNDS 16

SoundManager* soundManager;

SoundManager::SoundManager()
{
    //By creating a SoundBuffer we force SFML to load the sound subsystem. Else this is done when the first sound is loaded, causing a delay at that point.
    sf::SoundBuffer forceLoadBuffer;

    for(unsigned int n=0; n<MAX_SOUNDS; n++)
        activeSoundList.push_back(sf::Sound());
    
    music_volume = 100.0;
    positional_sound_enabled = false;
    music_channel.mode = None;
}

SoundManager::~SoundManager()
{
}

void SoundManager::playMusic(string name)
{
    music_set.clear();
    P<ResourceStream> stream = getResourceStream(name);
    if (stream)
        startMusic(stream, true);
}

void SoundManager::playMusicSet(std::vector<string> filenames)
{
    music_set = filenames;
    if (music_set.size() > 0)
        startMusic(getResourceStream(music_set[irandom(0, music_set.size() - 1)]), false);
    else
        stopMusic();
}

void SoundManager::stopMusic()
{
    music_set.clear();
    music_channel.music.stop();
}

void SoundManager::setMusicVolume(float volume)
{
    if (music_volume != volume)
    {
        music_volume = volume;
        if (music_channel.mode == None && music_channel.music.getStatus() != sf::Music::Stopped)
            music_channel.music.setVolume(music_volume);
    }
}

float SoundManager::getMusicVolume()
{
    return music_volume;
}

void SoundManager::playSound(string name, float pitch, float volume)
{
    sf::SoundBuffer* data = soundMap[name];
    if (data == NULL)
        data = loadSound(name);
    
    playSoundData(data, pitch, volume);
}

void SoundManager::setListenerPosition(sf::Vector2f position, float angle)
{
    sf::Vector2f listen_vector = sf::vector2FromAngle(angle);
    sf::Listener::setPosition(position.x, 0, position.y);
    sf::Listener::setDirection(listen_vector.x, 0, listen_vector.y);
    positional_sound_enabled = true;
}

void SoundManager::disablePositionalSound()
{
    positional_sound_enabled = false;
}

void SoundManager::playSound(string name, sf::Vector2f position, float min_distance, float attenuation, float pitch, float volume)
{
    if (!positional_sound_enabled)
        return;
    sf::SoundBuffer* data = soundMap[name];
    if (data == NULL)
        data = loadSound(name);
    if (data->getChannelCount() > 1)
        LOG(WARNING) << name << ": Used as positional sound but has more then 1 channel.";
    
    for(unsigned int n=0; n<activeSoundList.size(); n++)
    {
        sf::Sound& sound = activeSoundList[n];
        if (sound.getStatus() == sf::Sound::Stopped)
        {
            sound.setBuffer(*data);
            sound.setRelativeToListener(false);
            sound.setMinDistance(min_distance);
            sound.setAttenuation(attenuation);
            sound.setPosition(position.x, 0, position.y);
            sound.setPitch(pitch);
            sound.setVolume(volume);
            sound.play();
            return;
        }
    }
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
            sound.setRelativeToListener(true);
            sound.setMinDistance(1);
            sound.setAttenuation(0);
            sound.setPitch(pitch);
            sound.setVolume(volume);
            sound.setPosition(0, 0, 0);
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
        LOG(WARNING) << "Failed to load sound: " << name;
        soundMap[name] = data;
        return data;
    }
    
    LOG(INFO) << "Loaded: " << name << " of " << data->getDuration().asSeconds() << " seconds";
    soundMap[name] = data;
    return data;
}

void SoundManager::startMusic(P<ResourceStream> stream, bool loop)
{
    if (!stream)
        return;
    
    if (music_channel.music.getStatus() == sf::Music::Playing)
    {
        music_channel.next_stream = stream;
        music_channel.mode = FadeOut;
        music_channel.fade_delay = fade_music_time;
    }else{
        music_channel.mode = FadeIn;
        music_channel.fade_delay = fade_music_time;

        music_channel.music.openFromStream(**stream);
        music_channel.stream = stream;
        music_channel.music.setLoop(loop);
        music_channel.music.setVolume(0);
        music_channel.music.play();
    }
}

void SoundManager::updateTick()
{
    float delta = clock.restart().asSeconds();
    updateChannel(music_channel, delta);
    
    if (music_set.size() > 0)
    {
        if (music_channel.music.getStatus() == sf::Music::Playing && music_channel.mode == None)
        {
            if (!music_channel.next_stream)
            {
                if (music_channel.music.getPlayingOffset() > music_channel.music.getDuration() - sf::seconds(fade_music_time))
                {
                    startMusic(getResourceStream(music_set[irandom(0, music_set.size() - 1)]), false);
                }
            }
        }
    }
}

void SoundManager::updateChannel(MusicChannel& channel, float delta)
{
    switch(channel.mode)
    {
    case None:
        break;
    case FadeIn:
        channel.fade_delay -= delta;
        if (channel.fade_delay > 0.0)
        {
            channel.music.setVolume(music_volume * (1.0 - (channel.fade_delay / fade_music_time)));
        }else{
            channel.music.setVolume(music_volume);
            channel.mode = None;
        }
        break;
    case FadeOut:
        channel.fade_delay -= delta;
        if (channel.fade_delay > 0.0)
        {
            channel.music.setVolume(music_volume * (channel.fade_delay / fade_music_time));
        }else{
            channel.music.stop();
            channel.mode = None;
            if (channel.next_stream)
            {
                channel.music.openFromStream(**channel.next_stream);
                channel.stream = channel.next_stream;
                channel.next_stream = nullptr;
                channel.mode = FadeIn;
                channel.fade_delay = fade_music_time;
                channel.music.setVolume(0);
                channel.music.play();
            }
        }
        break;
    }
}
